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
#ifndef PXRUSDMAYA_SHADINGMODEEXPORTERCONTEXT_H
#define PXRUSDMAYA_SHADINGMODEEXPORTERCONTEXT_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/util.h"
#include "pxr/usd/usd/stage.h"


#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

class PxrUsdMayaShadingModeExportContext
{
public:
    void SetShadingEngine(MObject shadingEngine) { _shadingEngine = shadingEngine; }
    MObject GetShadingEngine() const { return _shadingEngine; }
    const UsdStageRefPtr& GetUsdStage() const { return _stage; }
    bool GetMergeTransformAndShape() const { return _mergeTransformAndShape; }
    const SdfPath& GetOverrideRootPath() const { return _overrideRootPath; }
    const SdfPathSet& GetBindableRoots() const { return _bindableRoots; }

    const PxrUsdMayaUtil::MDagPathMap<SdfPath>::Type& GetDagPathToUsdMap() const
    { return _dagPathToUsdMap; }

    PXRUSDMAYA_API
    MObject GetSurfaceShader() const;

    /// An assignment contains a bound prim path and a list of faceIndices. 
    /// If the list of faceIndices is non-empty, then it is a partial assignment
    /// targeting a subset of the bound prim's faces. 
    /// If the list of faceIndices is empty, it means the assignment targets 
    /// all the faces in the bound prim or the entire bound prim.
    typedef std::pair<SdfPath, VtIntArray> Assignment;
    
    /// Vector of assignments.
    typedef std::vector<Assignment> AssignmentVector;

    /// Returns a vector of binding assignments associated with the shading 
    /// engine.
    PXRUSDMAYA_API
    AssignmentVector GetAssignments() const;

    /// Use this function to create a UsdShadeMaterial prim at the "standard"
    /// location.  The "standard" location may change depending on arguments
    /// that are passed to the export script.
    /// 
    /// If \p boundPrimPaths is not NULL, it is populated with the set of 
    /// prim paths that were bound to the created material prim, based on the 
    /// given \p assignmentsToBind.
    PXRUSDMAYA_API
    UsdPrim MakeStandardMaterialPrim(
            const AssignmentVector& assignmentsToBind,
            const std::string& name=std::string(),
            SdfPathSet * const boundPrimPaths=nullptr) const;

    /// Use this function to get a "standard" usd attr name for \p attrPlug.
    /// The definition of "standard" may depend on arguments passed to the
    /// script (i.e. stripping namespaces, etc.).
    ///
    /// If attrPlug is an element in an array and if \p allowMultiElementArrays
    /// is true, this will <attrName>_<idx>.
    ///
    /// If it's false, this will return <attrName> if it's the 0-th logical
    /// element and an empty token otherwise.
    PXRUSDMAYA_API
    std::string GetStandardAttrName(
            const MPlug& attrPlug,
            bool allowMultiElementArrays) const;

    PXRUSDMAYA_API
    PxrUsdMayaShadingModeExportContext(
            const MObject& shadingEngine,
            const UsdStageRefPtr& stage,
            bool mergeTransformAndShape,
            bool stripNamespaces,
            const PxrUsdMayaUtil::ShapeSet& bindableRoots,
            SdfPath overrideRootPath,
            const PxrUsdMayaUtil::MDagPathMap<SdfPath>::Type& dagPathToUsdMap);
private:
    MObject _shadingEngine;
    const UsdStageRefPtr& _stage;
    bool _mergeTransformAndShape;
    bool _stripNamespaces;
    SdfPath _overrideRootPath;
    SdfPathSet _bindableRoots;
    const PxrUsdMayaUtil::MDagPathMap<SdfPath>::Type& _dagPathToUsdMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_SHADINGMODEEXPORTERCONTEXT_H
