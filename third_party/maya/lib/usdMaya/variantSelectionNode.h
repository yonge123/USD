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

#ifndef PXRUSDMAYA_VARIANTSELECTIONNODE_H
#define PXRUSDMAYA_VARIANTSELECTIONNODE_H

#include "pxr/pxr.h"

#include <maya/MPxNode.h>
#include <maya/MTypeId.h>
#include <maya/MString.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

// \brief Type to hold variants selections for all nodes in a scene.
typedef std::map<std::string, std::map<std::string, std::string>> VariantMapByPath;


/// \brief Node to hold information about variant selections.
///
/// Because the file translator executes in a context where it doesn't know
/// about it's reference, and any corresponding reference edits, (the reference
/// node may not even be created yet), usd references need an "external"
/// location to read variant selections from in order for them to be used while
/// the usd stage is loading (and therefore avoid having to load with "default"
/// variant set selections, and then reload later with the "correct" / "current"
/// variant sets). This node provides a canonical location for this information.
///
/// Note that for the same reasons, the node needs to store variant selection
/// information for the entire usd tree, not just "top level" usd references.
class UsdMayaVariantSelectionNode : public MPxNode {
public:

    struct PluginStaticData
    {
        // these get set in initialize()
        MObject selections;

        // this will not change once constructed.
        const MTypeId typeId;
        const MString typeName;

        PluginStaticData(
                const MTypeId& typeId,
                const MString& typeName) :
            typeId(typeId),
            typeName(typeName)
        { }
    };

//    virtual MStatus compute(const MPlug& plug, MDataBlock& data);
    static void*   creator(
            const PluginStaticData& psData);
    static MStatus initialize(
            PluginStaticData* psData);

private:
    const PluginStaticData& _psData;

    UsdMayaVariantSelectionNode(const PluginStaticData& psData);

    UsdMayaVariantSelectionNode(const UsdMayaVariantSelectionNode&);
//    virtual ~UsdMayaVariantSelectionNode();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_VARIANTSELECTIONNODE_H
