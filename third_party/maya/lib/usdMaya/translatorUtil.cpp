//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"
#include "usdMaya/translatorUtil.h"

#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"
#include "usdMaya/translatorXformable.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/xformable.h"

#include <maya/MDagModifier.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MNamespace.h>

PXR_NAMESPACE_OPEN_SCOPE



/* static */
bool
PxrUsdMayaTranslatorUtil::CreateTransformNode(
        const UsdPrim& usdPrim,
        MObject& parentNode,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context,
        MStatus* status,
        MObject* mayaNodeObj)
{
    static const MString _defaultTypeName("transform");

    if (!usdPrim || !usdPrim.IsA<UsdGeomXformable>()) {
        return false;
    }

    if (!CreateNode(usdPrim,
                       _defaultTypeName,
                       parentNode,
                       context,
                       status,
                       mayaNodeObj)) {
        return false;
    }

    // Read xformable attributes from the UsdPrim on to the transform node.
    UsdGeomXformable xformable(usdPrim);
    PxrUsdMayaTranslatorXformable::Read(xformable, *mayaNodeObj, args, context);

    return true;
}

/* static */
bool
PxrUsdMayaTranslatorUtil::CreateNode(
        const UsdPrim& usdPrim,
        const MString& nodeTypeName,
        MObject& parentNode,
        PxrUsdMayaPrimReaderContext* context,
        MStatus* status,
        MObject* mayaNodeObj)
{
    if (!CreateNode(MString(usdPrim.GetName().GetText()),
                       nodeTypeName,
                       parentNode,
                       status,
                       mayaNodeObj)) {
        return false;
    }

    if (context) {
        context->RegisterNewMayaNode(usdPrim.GetPath().GetString(), *mayaNodeObj);
    }

    return true;
}

/* static */
bool
PxrUsdMayaTranslatorUtil::CreateNode(
        const MString& nodeName,
        const MString& nodeTypeName,
        MObject& parentNode,
        MStatus* status,
        MObject* mayaNodeObj)
{
    // XXX:
    // Using MFnDagNode::create() results in nodes that are not properly
    // registered with parent scene assemblies. For now, just massaging the
    // transform code accordingly so that child scene assemblies properly post
    // their edits to their parents-- if this is indeed the best pattern for
    // this, all Maya*Reader node creation needs to be adjusted accordingly (for
    // much less trivial cases like MFnMesh).
    MDagModifier dagMod;
    *mayaNodeObj = dagMod.createNode(nodeTypeName, parentNode, status);
    CHECK_MSTATUS_AND_RETURN(*status, false);
    *status = dagMod.renameNode(*mayaNodeObj, nodeName);
    CHECK_MSTATUS_AND_RETURN(*status, false);
    *status = dagMod.doIt();
    CHECK_MSTATUS_AND_RETURN(*status, false);

    return TF_VERIFY(!mayaNodeObj->isNull());
}

/* static */
std::string
PxrUsdMayaTranslatorUtil::GetNamespace(
        const std::string& primPathStr, bool trailingColon)
{
    // TODO: make the root namespace a configurable option

    // We need a "root" namespace because otherwise we will have naming
    // conflicts with dg/dagNodes.  Ie, no top-level namespace can share the
    // name with ANY dg/dagNode (even dagNodes which aren't parented under the
    // world). However, there does not seem to be a restriction on sub-
    // namespace... so we create a top-level namespace to avoid conflicts.
    std::string rootNS = ":usd";
    std::string nameSpace;
    nameSpace.reserve(rootNS.length() + 1 + primPathStr.length() + 1);
    std::replace(nameSpace.begin(), nameSpace.end(), '/', ':');
    nameSpace = rootNS;
    nameSpace += ":";
    nameSpace += TfStringTrim(primPathStr, "/:");
    std::replace(nameSpace.begin(), nameSpace.end(), '/', ':');
    if (!trailingColon)
    {
        while(nameSpace.back() == ':')
        {
            nameSpace.pop_back();
        }
    }
    else
    {
        if (nameSpace.back() != ':')
        {
            nameSpace += ':';
        }
    }
    return nameSpace;
}

/* static */
std::string
PxrUsdMayaTranslatorUtil::GetNamespace(
        const SdfPath& primPath, bool trailingColon)
{
    return GetNamespace(primPath.GetString(), trailingColon);
}

/* static */
std::string
PxrUsdMayaTranslatorUtil::GetParentNamespace(
        const std::string& primPathStr, bool trailingColon)
{
    // ensure no leading/trailing "/", so TfGetPathName return nothing if no parent
    return GetNamespace(TfGetPathName(TfStringTrim(primPathStr, "/")), trailingColon);
}

/* static */
std::string
PxrUsdMayaTranslatorUtil::GetParentNamespace(
        const SdfPath& primPath, bool trailingColon)
{
    return GetNamespace(primPath.GetParentPath(), trailingColon);
}

/* static */
bool
PxrUsdMayaTranslatorUtil::CreateNamespace(const std::string& fullNamespace)
{
    std::string absoluteNS = std::string(":") + TfStringTrim(fullNamespace, ":");
    if (absoluteNS.length() == 1) return true;

    if(MNamespace::namespaceExists(absoluteNS.c_str())) return true;

    std::string finalNS = TfStringGetSuffix(absoluteNS, ':');
    std::string parentNS;
    if (absoluteNS.length() > (finalNS.length() + 1))
    {
        std::string parentNS = absoluteNS.substr(0, absoluteNS.length()
                                                    - finalNS.length() - 1);
        if (!CreateNamespace(parentNS)) return false;
        MString parentMstring(parentNS.c_str());
        CHECK_MSTATUS_AND_RETURN(MNamespace::addNamespace(finalNS.c_str(),
                                                          &parentMstring),
                                 false);
    }
    else
    {
        CHECK_MSTATUS_AND_RETURN(MNamespace::addNamespace(finalNS.c_str()),
                                 false);
    }
    return true;
}

/* static */
bool
PxrUsdMayaTranslatorUtil::CreateParentNamespace(const std::string& fullNamespace)
{
    // ensure ns starts with ":", so TfStringGetBeforeSuffix return nothing if no parent
    std::string absoluteNS = std::string(":") + TfStringTrim(fullNamespace, ":");
    return CreateNamespace(TfStringGetBeforeSuffix(absoluteNS, ':'));
}

PXR_NAMESPACE_CLOSE_SCOPE

