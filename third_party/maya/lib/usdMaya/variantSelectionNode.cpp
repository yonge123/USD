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
#include "usdMaya/variantSelectionNode.h"

#include <maya/MFnTypedAttribute.h>

PXR_NAMESPACE_OPEN_SCOPE

/* static */
void*
UsdMayaVariantSelectionNode::creator(
        const PluginStaticData& psData)
{
    return new UsdMayaVariantSelectionNode(psData);
}


/* static */
MStatus
UsdMayaVariantSelectionNode::initialize(
        PluginStaticData* psData)
{
    MStatus retValue = MS::kSuccess;

    MFnTypedAttribute typedAttrFn;

    // Holds a json string representing our selections, which looks like:
    //
    // {
    //     "/usd/path/to/node": {"variantSetName1": "variantValue1", ...},
    //     ...,
    // }
    //

    // TOODO: eventually make this attr into a custom MPxData type, for faster
    // modification.  There could potentially be a lot of selections here
    // (especially since it will hold information for the entire subtree), so
    // we need to be able to access selections for individual nodes efficiently,
    // ie, without having to read/write the entire string for ALL variant set
    // selections on ALL nodes.
    // Also keep a read-only string version of the attribute, however, for easy
    // reading / debugging by end users. All programmed interaction should use
    // the MPxData type, though. May need to make a MPxCommand to facilitate
    // this...
    psData->selections = typedAttrFn.create(
        "selections",
        "sl",
        MFnData::kString,
        MObject::kNullObj,
        &retValue);
    typedAttrFn.setCached(true);
    typedAttrFn.setReadable(true);
    typedAttrFn.setStorable(true);
    typedAttrFn.setWritable(true);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);
    retValue = addAttribute(psData->selections);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    return retValue;
}

UsdMayaVariantSelectionNode::UsdMayaVariantSelectionNode(
        const PluginStaticData& psData)
    : _psData(psData)
{
}



PXR_NAMESPACE_CLOSE_SCOPE
