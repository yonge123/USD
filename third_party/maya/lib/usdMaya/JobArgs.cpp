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
#include "usdMaya/JobArgs.h"

#include "usdMaya/shadingModeRegistry.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <maya/MDagPath.h>
#include <maya/MNodeClass.h>
#include <maya/MTypeId.h>

#include <ostream>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE



TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaTranslatorTokens,
        PXRUSDMAYA_TRANSLATOR_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(PxUsdExportJobArgsTokens, 
        PXRUSDMAYA_JOBARGS_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _defaultIncludeMetadataKeys, 
    (hidden)
    (instanceable)
    (kind)
);

JobExportArgs::JobExportArgs()
    :
        exportRefsAsInstanceable(false),
        exportDisplayColor(true),
        shadingMode(PxrUsdMayaShadingModeTokens->displayColor),
        mergeTransformAndShape(true),
        exportInstances(true),
        timeInterval(/* empty interval (no animation) */),
        exportAsClip(false),
        clipStartTime(UsdTimeCode::Default()),
        eulerFilter(false),
        excludeInvisible(false),
        exportDefaultCameras(false),
        exportSkin(false),
        autoSkelRoots(false),
        exportMeshUVs(true),
        normalizeMeshUVs(false),
        exportMaterialCollections(false),
        materialCollectionsPath(""),
        exportCollectionBasedBindings(false),
        normalizeNurbs(false),
        exportNurbsExplicitUV(true),
        exportColorSets(true),
        exportReferenceObjects(false),
        renderLayerMode(PxUsdExportJobArgsTokens->defaultLayer),
        defaultMeshScheme(UsdGeomTokens->catmullClark),
        exportVisibility(true),
        stripNamespaces(false),
        exportRootPath(""),
        parentScope(SdfPath())
{
}

std::ostream&
operator <<(std::ostream& out, const JobExportArgs& exportArgs)
{
    out << "exportRefsAsInstanceable: " << TfStringify(exportArgs.exportRefsAsInstanceable) << std::endl
        << "exportDisplayColor: " << TfStringify(exportArgs.exportDisplayColor) << std::endl
        << "shadingMode: " << exportArgs.shadingMode << std::endl
        << "mergeTransformAndShape: " << TfStringify(exportArgs.mergeTransformAndShape) << std::endl
        << "exportInstances: " << TfStringify(exportArgs.exportInstances) << std::endl
        << "timeInterval: " << exportArgs.timeInterval << std::endl
        << "exportAsClip: " << TfStringify(exportArgs.exportAsClip) << std::endl
        << "clipStartTime: " << exportArgs.clipStartTime << std::endl
        << "eulerFilter: " << TfStringify(exportArgs.eulerFilter) << std::endl
        << "excludeInvisible: " << TfStringify(exportArgs.excludeInvisible) << std::endl
        << "exportDefaultCameras: " << TfStringify(exportArgs.exportDefaultCameras) << std::endl
        << "exportSkin: " << TfStringify(exportArgs.exportSkin) << std::endl
        << "autoSkelRoots: " << TfStringify(exportArgs.autoSkelRoots) << std::endl
        << "exportMeshUVs: " << TfStringify(exportArgs.exportMeshUVs) << std::endl
        << "normalizeMeshUVs: " << TfStringify(exportArgs.normalizeMeshUVs) << std::endl
        << "exportMaterialCollections: " << TfStringify(exportArgs.exportMaterialCollections) << std::endl
        << "materialCollectionsPath: " << exportArgs.materialCollectionsPath << std::endl
        << "exportCollectionBasedBindings: " << TfStringify(exportArgs.exportCollectionBasedBindings) << std::endl
        << "normalizeNurbs: " << TfStringify(exportArgs.normalizeNurbs) << std::endl
        << "exportNurbsExplicitUV: " << TfStringify(exportArgs.exportNurbsExplicitUV) << std::endl
        << "exportColorSets: " << TfStringify(exportArgs.exportColorSets) << std::endl
        << "renderLayerMode: " << exportArgs.renderLayerMode << std::endl
        << "defaultMeshScheme: " << exportArgs.defaultMeshScheme << std::endl
        << "exportVisibility: " << TfStringify(exportArgs.exportVisibility) << std::endl
        << "parentScope: " << exportArgs.getParentScope() << std::endl;

    out << "melPerFrameCallback: " << exportArgs.melPerFrameCallback << std::endl
        << "melPostCallback: " << exportArgs.melPostCallback << std::endl
        << "pythonPerFrameCallback: " << exportArgs.pythonPerFrameCallback << std::endl
        << "pythonPostCallback: " << exportArgs.pythonPostCallback << std::endl;

    out << "dagPaths (" << exportArgs.dagPaths.size() << ")" << std::endl;
    for (const MDagPath& dagPath : exportArgs.dagPaths) {
        out << "    " << dagPath.fullPathName().asChar() << std::endl;
    }
    out << "filteredTypeIds (" << exportArgs.getFilteredTypeIds().size() << ")" << std::endl;
    for (unsigned int id : exportArgs.getFilteredTypeIds()) {
        out << "    " << id << ": " << MNodeClass(MTypeId(id)).className() << std::endl;
    }

    out << "chaserNames (" << exportArgs.chaserNames.size() << ")" << std::endl;
    for (const std::string& chaserName : exportArgs.chaserNames) {
        out << "    " << chaserName << std::endl;        
    }

    out << "allChaserArgs (" << exportArgs.allChaserArgs.size() << ")" << std::endl;
    for (const auto& chaserIter : exportArgs.allChaserArgs) {
        // Chaser name.
        out << "    " << chaserIter.first << std::endl;

        for (const auto& argIter : chaserIter.second) {
            out << "        Arg Name: " << argIter.first
                << ", Value: " << argIter.second << std::endl;
        }
    }

    out << "usdModelRootOverridePath: " << exportArgs.usdModelRootOverridePath << std::endl
        << "rootKind: " << exportArgs.rootKind << std::endl;

    return out;
}

void JobExportArgs::setParentScope(const std::string& ps) {
    // Otherwise this is a malformed sdfpath.
    if (!ps.empty()) {
        parentScope = ps[0] == '/' ? SdfPath(ps) : SdfPath("/" + ps);
    }
}

void JobExportArgs::addFilteredTypeName(const MString& typeName)
{
    MNodeClass cls(typeName);
    unsigned int id = cls.typeId().id();
    if (id == 0) {
        MGlobal::displayWarning(MString("Given excluded node type '") + typeName
                + "' does not exist; ignoring");
        return;
    }
    filteredTypeIds.insert(id);
    // We also insert all inherited types - only way to query this is through mel,
    // which is slower, but this should be ok, as these queries are only done
    // "up front" when the export starts, not per-node
    MString queryCommand("nodeType -isTypeName -derived ");
    queryCommand += typeName;
    MStringArray inheritedTypes;
    MStatus status = MGlobal::executeCommand(queryCommand, inheritedTypes, false, false);
    if (!status) {
        MGlobal::displayWarning(MString("Error querying derived types for '") + typeName
                + "': " + status.errorString());
        return;
    }

    for (unsigned int i=0; i < inheritedTypes.length(); ++i) {
        if (inheritedTypes[i].length() == 0) continue;
        id = MNodeClass(inheritedTypes[i]).typeId().id();
        if (id == 0) {
            // Unfortunately, the returned list will often include weird garbage, like
            // "THconstraint" for "constraint", which cannot be converted to a MNodeClass,
            // so just ignore these...
            // MGlobal::displayError(MString("Given inherited excluded node type '") + inheritedTypes[i]
            //         + "' does not exist; ignoring");
            continue;
        }
        filteredTypeIds.insert(id);
    }
}

JobImportArgs::JobImportArgs()
    :
        shadingMode(PxrUsdMayaShadingModeTokens->displayColor),
        assemblyRep(PxrUsdMayaTranslatorTokens->Collapsed),
        timeInterval(GfInterval::GetFullInterval()),
        importWithProxyShapes(false),
        includeMetadataKeys(
                _defaultIncludeMetadataKeys->allTokens.begin(),
                _defaultIncludeMetadataKeys->allTokens.end()),
        includeAPINames(/*empty*/)
{
}

std::ostream&
operator <<(std::ostream& out, const JobImportArgs& importArgs)
{
    out << "shadingMode: " << importArgs.shadingMode << std::endl
        << "assemblyRep: " << importArgs.assemblyRep << std::endl
        << "timeInterval: " << importArgs.timeInterval << std::endl
        << "importWithProxyShapes: " << TfStringify(importArgs.importWithProxyShapes) << std::endl;

    return out;
}


PXR_NAMESPACE_CLOSE_SCOPE
