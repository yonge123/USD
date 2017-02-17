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
#include "usdMaya/translatorMaterial.h"

#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/base/gf/matrix4f.h"

#include <maya/MFnMeshData.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>

#include <dlfcn.h>

PXR_NAMESPACE_OPEN_SCOPE

#define AI_NODE_SHADER      0x0010


TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaTranslatorMaterialTokens,
    PXRUSDMAYA_TRANSLATOR_MATERIAL_TOKENS);


/* static */
MObject
PxrUsdMayaTranslatorMaterial::Read(
        const TfToken& shadingMode,
        const UsdShadeMaterial& shadeMaterial,
        const UsdGeomGprim& boundPrim,
        PxrUsdMayaPrimReaderContext* context)
{
    if (shadingMode == PxrUsdMayaShadingModeTokens->none) {
        return MObject();
    }

    PxrUsdMayaShadingModeImportContext c(shadeMaterial, boundPrim, context);

    MObject shadingEngine;

    if (c.GetCreatedObject(shadeMaterial.GetPrim(), &shadingEngine)) {
        return shadingEngine;
    }

    MStatus status;

    MPlug outColorPlug;

    if (PxrUsdMayaShadingModeImporter importer = 
            PxrUsdMayaShadingModeRegistry::GetImporter(shadingMode)) {
        outColorPlug = importer(&c);
    }
    else {
        // this could spew a lot so we don't warn here.  Ideally, we did some
        // validation up front.
    }

    if (!outColorPlug.isNull()) {
        MFnSet fnSet;
        MSelectionList tmpSelList;
        shadingEngine = fnSet.create(tmpSelList, MFnSet::kRenderableOnly, &status);

        // To make sure that the shadingEngine object names do not collide with
        // the Maya transform or shape node names, we put the shadingEngine
        // objects into their own namespace.
        std::string shadingEngineName =
            PxrUsdMayaTranslatorMaterialTokens->MaterialNamespace.GetString() + std::string(":") +
            (shadeMaterial ? shadeMaterial.GetPrim() : boundPrim.GetPrim()
                ).GetName().GetString();

        if (!status) {
            MGlobal::displayError(TfStringPrintf(
                        "Failed to make shadingEngine for %s\n", 
                        shadingEngineName.c_str()).c_str());
            return shadingEngine;
        }
        fnSet.setName(MString(shadingEngineName.c_str()),
            true /* createNamespace */);

        MPlug seSurfaceShaderPlg = fnSet.findPlug("surfaceShader", &status);
        PxrUsdMayaUtil::Connect(outColorPlug, seSurfaceShaderPlg, 
                // Make sure that "surfaceShader" connection is open
                true);
    }

    return c.AddCreatedObject(shadeMaterial.GetPrim(), shadingEngine);
}

static 
bool 
_AssignMaterialFaceSet(const MObject &shadingEngine,
                   const MDagPath &shapeDagPath,
                   const VtIntArray &faceIndices)
{
    MStatus status;

    // Create component object using single indexed  
    // components, i.e. face indices.
    MFnSingleIndexedComponent compFn;
    MObject faceComp = compFn.create(
        MFn::kMeshPolygonComponent, &status);
    if (!status) {
        MGlobal::displayError("Failed to create face component.");
        return false;
    }

    MIntArray mFaces;
    TF_FOR_ALL(fIdxIt, faceIndices) {
        mFaces.append(*fIdxIt);
    }
    compFn.addElements(mFaces);

    MFnSet seFnSet(shadingEngine, &status);
    if (seFnSet.restriction() == MFnSet::kRenderableOnly) {
        status = seFnSet.addMember(shapeDagPath, faceComp);
        if (!status) {
            MGlobal::displayError(TfStringPrintf("Could not"
                " add component to shadingEngine %s.", 
                seFnSet.name().asChar()).c_str());
            return false;
        }
    }

    return true;
}

bool
PxrUsdMayaTranslatorMaterial::AssignMaterial(
        const TfToken& shadingMode,
        const UsdGeomGprim& primSchema,
        MObject shapeObj,
        PxrUsdMayaPrimReaderContext* context)
{
    // if we don't have a valid context, we make one temporarily.  This is to
    // make sure we don't duplicate shading nodes within a material.
    PxrUsdMayaPrimReaderContext::ObjectRegistry tmpRegistry;
    PxrUsdMayaPrimReaderContext tmpContext(&tmpRegistry);
    if (!context) {
        context = &tmpContext;
    }

    MDagPath shapeDagPath;
    MFnDagNode(shapeObj).getPath(shapeDagPath);

    MStatus status;
    MObject shadingEngine = PxrUsdMayaTranslatorMaterial::Read(
            shadingMode,
            UsdShadeMaterial::GetBoundMaterial(primSchema.GetPrim()), 
            primSchema, 
            context);

    if (shadingEngine.isNull()) {
        status = PxrUsdMayaUtil::GetMObjectByName("initialShadingGroup",
                                                  shadingEngine);
        if (status != MS::kSuccess) {
            return false;
        }
    }

    // If the gprim does not have a material faceSet which represents per-face 
    // shader assignments, assign the shading engine to the entire gprim.
    if (!UsdShadeMaterial::HasMaterialFaceSet(primSchema.GetPrim())) {
        MFnSet seFnSet(shadingEngine, &status);
        if (seFnSet.restriction() == MFnSet::kRenderableOnly) {
            status = seFnSet.addMember(shapeObj);
            if (!status) {
                MGlobal::displayError("Could not add shapeObj to shadingEngine.\n");
            }
        }

        return true;
    } 

    // Import per-face-set shader bindings.    
    UsdGeomFaceSetAPI materialFaceSet = UsdShadeMaterial::GetMaterialFaceSet(
        primSchema.GetPrim());

    SdfPathVector bindingTargets;
    if (!materialFaceSet.GetBindingTargets(&bindingTargets) ||
        bindingTargets.empty()) {
            
        MGlobal::displayWarning(TfStringPrintf("No bindings found on material "
            "faceSet at path <%s>.", primSchema.GetPath().GetText()).c_str());
        // No bindings to export in the material faceSet.
        return false;
    }

    std::string reason;
    if (!materialFaceSet.Validate(&reason)) {
        MGlobal::displayWarning(TfStringPrintf("Invalid faceSet data "
            "found on <%s>: %s", primSchema.GetPath().GetText(), 
            reason.c_str()).c_str());
        return false;
    }

    VtIntArray faceCounts, faceIndices;
    bool isPartition = materialFaceSet.GetIsPartition();
    materialFaceSet.GetFaceCounts(&faceCounts);
    materialFaceSet.GetFaceIndices(&faceIndices);

    if (!isPartition) {
        MGlobal::displayWarning(TfStringPrintf("Invalid faceSet data "
            "found on <%s>: Not a partition.", 
            primSchema.GetPath().GetText()).c_str());
        return false;
    }

    // Check if there are faceIndices that aren't included in the material
    // face-set. 
    // Note: This won't occur if the shading was originally 
    // authored in maya and exported to the USD that we are importing, 
    // but this is supported by the USD shading model.
    UsdGeomMesh mesh(primSchema);
    if (mesh) {                
        VtIntArray faceVertexCounts;
        if (mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts))
        {
            std::set<int> assignedIndices(faceIndices.begin(),
                                            faceIndices.end());
            VtIntArray unassignedIndices;
            unassignedIndices.reserve(faceVertexCounts.size() -
                                        faceIndices.size());
            for(size_t fIdx = 0; fIdx < faceVertexCounts.size(); fIdx++) 
            {
                if (assignedIndices.count(fIdx) == 0) {
                    unassignedIndices.push_back(fIdx);
                }
            }

            // Assign the face face indices that aren't in the material
            // faceSet to the material that the mesh is bound to or 
            // to the initialShadingGroup if it doesn't have a material 
            // binding.
            if (!unassignedIndices.empty()) {
                if (!_AssignMaterialFaceSet(shadingEngine, shapeDagPath, 
                                           unassignedIndices)) {
                    return false;
                }
            }
        }
    }

    int setIndex = 0;
    int currentFaceIndex = 0;
    TF_FOR_ALL(bindingTargetsIt, bindingTargets) {
        UsdShadeMaterial material(
            primSchema.GetPrim().GetStage()->GetPrimAtPath(
                *bindingTargetsIt));

        MObject faceGroupShadingEngine = PxrUsdMayaTranslatorMaterial::Read(
            shadingMode, material, UsdGeomGprim(), context);
 
        if (faceGroupShadingEngine.isNull()) {
            status = PxrUsdMayaUtil::GetMObjectByName("initialShadingGroup",
                                                      faceGroupShadingEngine);
            if (status != MS::kSuccess) {
                return false;
            }
        }

        int numFaces = faceCounts[setIndex];
        VtIntArray faceGroupIndices;
        faceGroupIndices.reserve(numFaces);
        for (int faceIndex = currentFaceIndex; 
            faceIndex < currentFaceIndex + numFaces;
            ++faceIndex) {
            faceGroupIndices.push_back(faceIndices[faceIndex]);
        }

        ++setIndex;
        currentFaceIndex += numFaces;

        if (!_AssignMaterialFaceSet(faceGroupShadingEngine, shapeDagPath, 
                                   faceGroupIndices)) {
            return false;
        }
    }

    return true;
}

namespace {
    struct AtNode;
    struct AtNodeEntry;
    struct AtUserParamIterator;
    struct AtUserParamEntry;
    struct AtParamIterator;
    struct AtParamEntry;
    struct AtArray;

    constexpr int32_t AI_TYPE_BYTE = 0x00;
    constexpr int32_t AI_TYPE_INT = 0x01;
    constexpr int32_t AI_TYPE_UINT = 0x02;
    constexpr int32_t AI_TYPE_BOOLEAN = 0x03;
    constexpr int32_t AI_TYPE_FLOAT = 0x04;
    constexpr int32_t AI_TYPE_RGB = 0x05;
    constexpr int32_t AI_TYPE_RGBA = 0x06;
    constexpr int32_t AI_TYPE_VECTOR = 0x07;
    constexpr int32_t AI_TYPE_POINT = 0x08;
    constexpr int32_t AI_TYPE_POINT2 = 0x09;
    constexpr int32_t AI_TYPE_STRING = 0x0A;
    constexpr int32_t AI_TYPE_POINTER = 0x0B;
    constexpr int32_t AI_TYPE_NODE = 0x0C;
    constexpr int32_t AI_TYPE_ARRAY = 0x0D;
    constexpr int32_t AI_TYPE_MATRIX = 0x0E;
    constexpr int32_t AI_TYPE_ENUM = 0x0F;
    constexpr int32_t AI_TYPE_UNDEFINED = 0xFF;
    constexpr int32_t AI_TYPE_NONE = 0xFF;

    // mtoa functions
    void (*mtoa_init_export_session) () = nullptr;
    AtNode* (*mtoa_export_node) (void*, const char*) = nullptr;
    void (*mtoa_destroy_export_session) () = nullptr;
    AtNode* (*AiNodeGetLink) (const AtNode*, const char*, int32_t*) = nullptr;
    const char* (*AiNodeGetName) (const AtNode*) = nullptr;
    const AtNodeEntry* (*AiNodeGetNodeEntry) (const AtNode*) = nullptr;
    AtUserParamIterator* (*AiNodeGetUserParamIterator) (const AtNode*) = nullptr;
    void (*AiUserParamIteratorDestroy) (AtUserParamIterator*) = nullptr;
    const AtUserParamEntry* (*AiUserParamIteratorGetNext) (AtUserParamIterator*) = nullptr;
    bool (*AiUserParamIteratorFinished) (const AtUserParamIterator*) = nullptr;
    const char* (*AiUserParamGetName) (const AtUserParamEntry*) = nullptr;
    int (*AiUserParamGetType) (const AtUserParamEntry*) = nullptr;
    int (*AiUserParamGetArrayType) (const AtUserParamEntry* upentry) = nullptr;
    const char* (*AiNodeEntryGetName) (const AtNodeEntry* nentry) = nullptr;
    AtParamIterator* (*AiNodeEntryGetParamIterator) (const AtNodeEntry* nentry) = nullptr;
    void (*AiParamIteratorDestroy) (AtParamIterator* iter) = nullptr;
    const AtParamEntry* (*AiParamIteratorGetNext) (AtParamIterator* iter) = nullptr;
    bool (*AiParamIteratorFinished) (const AtParamIterator* iter) = nullptr;
    const char* (*AiParamGetName) (const AtParamEntry* pentry) = nullptr;
    int (*AiParamGetType) (const AtParamEntry* pentry) = nullptr;
    int8_t (*AiNodeGetByte) (const AtNode*, const char*) = nullptr;
    int32_t (*AiNodeGetInt) (const AtNode*, const char*) = nullptr;
    uint32_t (*AiNodeGetUInt) (const AtNode*, const char*) = nullptr;
    bool (*AiNodeGetBool) (const AtNode*, const char*) = nullptr;
    float (*AiNodeGetFlt) (const AtNode*, const char*) = nullptr;
    GfVec3f (*AiNodeGetRGB) (const AtNode*, const char*) = nullptr;
    GfVec4f (*AiNodeGetRGBA) (const AtNode*, const char*) = nullptr;
    GfVec3f (*AiNodeGetVec) (const AtNode*, const char*) = nullptr;
    GfVec3f (*AiNodeGetPnt) (const AtNode*, const char*) = nullptr;
    GfVec2f (*AiNodeGetPnt2) (const AtNode*, const char*) = nullptr;
    const char* (*AiNodeGetStr) (const AtNode*, const char*) = nullptr;
    void* (*AiNodeGetPtr) (const AtNode*, const char*) = nullptr;
    AtArray* (*AiNodeGetArray) (const AtNode*, const char*) = nullptr;
    void (*_AiNodeGetMtx) (const AtNode*, const char*, float*) = nullptr;
    
    GfMatrix4f AiNodeGetMtx(const AtNode* node, const char* param) {
        GfMatrix4f ret;
        _AiNodeGetMtx(node, param, ret[0]);
        return ret;
    };

    template <typename T>
    void convert_ptr(T& trg, void* sym) {
        trg = reinterpret_cast<T>(sym);
    }

    void setup_pointers(void* mtoa_handle, void* ai_handle) {
        // mtoa functions
        convert_ptr(mtoa_init_export_session, dlsym(mtoa_handle, "mtoa_init_export_session"));
        convert_ptr(mtoa_export_node, dlsym(mtoa_handle, "mtoa_export_node"));
        convert_ptr(mtoa_destroy_export_session, dlsym(mtoa_handle, "mtoa_destroy_export_session"));
        convert_ptr(AiNodeGetLink, dlsym(ai_handle, "AiNodeGetLink"));
        convert_ptr(AiNodeGetName, dlsym(ai_handle, "AiNodeGetName"));
        convert_ptr(AiNodeGetNodeEntry, dlsym(ai_handle, "AiNodeGetNodeEntry"));
        convert_ptr(AiNodeGetUserParamIterator, dlsym(ai_handle, "AiNodeGetUserParamIterator"));
        convert_ptr(AiUserParamIteratorDestroy, dlsym(ai_handle, "AiUserParamIteratorDestroy"));
        convert_ptr(AiUserParamIteratorGetNext, dlsym(ai_handle, "AiUserParamIteratorGetNext"));
        convert_ptr(AiUserParamIteratorFinished, dlsym(ai_handle, "AiUserParamIteratorFinished"));
        convert_ptr(AiUserParamGetName, dlsym(ai_handle, "AiUserParamGetName"));
        convert_ptr(AiUserParamGetType, dlsym(ai_handle, "AiUserParamGetType"));
        convert_ptr(AiUserParamGetArrayType, dlsym(ai_handle, "AiUserParamGetArrayType"));
        convert_ptr(AiNodeEntryGetName, dlsym(ai_handle, "AiNodeEntryGetName"));
        convert_ptr(AiNodeEntryGetParamIterator, dlsym(ai_handle, "AiNodeEntryGetParamIterator"));
        convert_ptr(AiParamIteratorDestroy, dlsym(ai_handle, "AiParamIteratorDestory"));
        convert_ptr(AiParamIteratorGetNext, dlsym(ai_handle, "AiParamIteratorGetNext"));
        convert_ptr(AiParamIteratorFinished, dlsym(ai_handle, "AiParamIteratorFinished"));
        convert_ptr(AiParamGetName, dlsym(ai_handle, "AiParamGetName"));
        convert_ptr(AiParamGetType, dlsym(ai_handle, "AiParamGetType"));
        convert_ptr(AiNodeGetByte, dlsym(ai_handle, "AiNodeGetByte"));
        convert_ptr(AiNodeGetInt, dlsym(ai_handle, "AiNodeGetInt"));
        convert_ptr(AiNodeGetUInt, dlsym(ai_handle, "AiNodeGetUInt"));
        convert_ptr(AiNodeGetBool, dlsym(ai_handle, "AiNodeGetBool"));
        convert_ptr(AiNodeGetFlt, dlsym(ai_handle, "AiNodeGetFlt"));
        convert_ptr(AiNodeGetRGB, dlsym(ai_handle, "AiNodeGetRGB"));
        convert_ptr(AiNodeGetRGBA, dlsym(ai_handle, "AiNodeGetRGBA"));
        convert_ptr(AiNodeGetVec, dlsym(ai_handle, "AiNodeGetVec"));
        convert_ptr(AiNodeGetPnt, dlsym(ai_handle, "AiNodeGetPnt"));
        convert_ptr(AiNodeGetPnt2, dlsym(ai_handle, "AiNodeGetPnt2"));
        convert_ptr(AiNodeGetStr, dlsym(ai_handle, "AiNodeGetStr"));
        convert_ptr(AiNodeGetPtr, dlsym(ai_handle, "AiNodeGetPtr"));
        convert_ptr(AiNodeGetArray, dlsym(ai_handle, "AiNodeGetArray"));
        convert_ptr(_AiNodeGetMtx, dlsym(ai_handle, "AiNodeGetMtx"));
    }
}

/* static */
void
PxrUsdMayaTranslatorMaterial::ExportShadingEngines(
        const UsdStageRefPtr& stage,
        const PxrUsdMayaUtil::ShapeSet& bindableRoots,
        const TfToken& shadingMode,
        bool mergeTransformAndShape,
        SdfPath overrideRootPath)
{
    if (shadingMode == PxrUsdMayaShadingModeTokens->none) {
        return;
    }

    if (shadingMode == PxrUsdMayaShadingModeTokens->arnold) {
        // traditional registry doesn't work here, as it's only a simple function pointer
        // which doesn't play well with starting up and shutting down the mtoa exporter

        auto ai_handle = dlopen("libai.so", RTLD_LAZY | RTLD_GLOBAL);
        if (ai_handle == nullptr) { return; }
        std::string mtoa_path = std::string(getenv("MTOA_HOME")) + std::string("/plug-ins/mtoa.so");
        auto mtoa_handle = dlopen(mtoa_path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
        if (mtoa_handle == nullptr) { return; }
        // struct definitions
        setup_pointers(mtoa_handle, ai_handle);
        mtoa_init_export_session();
        for (MItDependencyNodes iter(MFn::kShadingEngine); !iter.isDone(); iter.next()) {
            MObject obj = iter.thisNode();
            auto arnoldNode = mtoa_export_node(&obj, "message");
            std::cerr << "Exported arnold node : " << AiNodeGetName(arnoldNode) << std::endl;
        }
        mtoa_destroy_export_session();

    } else if (auto exporterCreator =
            PxrUsdMayaShadingModeRegistry::GetExporter(shadingMode)) {
        if (auto exporter = exporterCreator()) {
            exporter->DoExport(stage, bindableRoots, mergeTransformAndShape, overrideRootPath);
        }
    }
    else {
        MGlobal::displayError(TfStringPrintf("No shadingMode '%s' found.",
                    shadingMode.GetText()).c_str());
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

