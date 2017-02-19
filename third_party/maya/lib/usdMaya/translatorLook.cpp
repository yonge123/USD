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
#include "usdMaya/translatorLook.h"

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


TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaTranslatorLookTokens,
    PXRUSDMAYA_TRANSLATOR_LOOK_TOKENS);


/* static */
MObject
PxrUsdMayaTranslatorLook::Read(
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
            PxrUsdMayaTranslatorLookTokens->LookNamespace.GetString() + std::string(":") +
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
PxrUsdMayaTranslatorLook::AssignLook(
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
    MObject shadingEngine = PxrUsdMayaTranslatorLook::Read(
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
    if (!materialFaceSet.GetBindingTargets(&bindingTargets) or
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

        MObject faceGroupShadingEngine = PxrUsdMayaTranslatorLook::Read(
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

#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdAi/aiShader.h"
#include "pxr/usd/usdAi/aiMaterialAPI.h"

namespace {
    class ArnoldShaderExport {
    private:
        struct AtNode;
        struct AtNodeEntry;
        struct AtUserParamIterator;
        struct AtUserParamEntry;
        struct AtParamIterator;
        struct AtParamEntry;
        struct AtArray;

        constexpr static int32_t AI_TYPE_BYTE = 0x00;
        constexpr static int32_t AI_TYPE_INT = 0x01;
        constexpr static int32_t AI_TYPE_UINT = 0x02;
        constexpr static int32_t AI_TYPE_BOOLEAN = 0x03;
        constexpr static int32_t AI_TYPE_FLOAT = 0x04;
        constexpr static int32_t AI_TYPE_RGB = 0x05;
        constexpr static int32_t AI_TYPE_RGBA = 0x06;
        constexpr static int32_t AI_TYPE_VECTOR = 0x07;
        constexpr static int32_t AI_TYPE_POINT = 0x08;
        constexpr static int32_t AI_TYPE_POINT2 = 0x09;
        constexpr static int32_t AI_TYPE_STRING = 0x0A;
        constexpr static int32_t AI_TYPE_POINTER = 0x0B;
        constexpr static int32_t AI_TYPE_NODE = 0x0C;
        constexpr static int32_t AI_TYPE_ARRAY = 0x0D;
        constexpr static int32_t AI_TYPE_MATRIX = 0x0E;
        constexpr static int32_t AI_TYPE_ENUM = 0x0F;
        constexpr static int32_t AI_TYPE_UNDEFINED = 0xFF;
        constexpr static int32_t AI_TYPE_NONE = 0xFF;

        struct ArnoldCtx {
            // mtoa functions
            void (*mtoa_init_export_session) () = nullptr;
            AtNode* (*mtoa_export_node) (void*, const char*) = nullptr;
            void (*mtoa_destroy_export_session) () = nullptr;
            AtNode* (*NodeGetLink) (const AtNode*, const char*, int32_t*) = nullptr;
            const char* (*NodeGetName) (const AtNode*) = nullptr;
            const AtNodeEntry* (*NodeGetNodeEntry) (const AtNode*) = nullptr;
            AtUserParamIterator* (*NodeGetUserParamIterator) (const AtNode*) = nullptr;
            void (*UserParamIteratorDestroy) (AtUserParamIterator*) = nullptr;
            const AtUserParamEntry* (*UserParamIteratorGetNext) (AtUserParamIterator*) = nullptr;
            bool (*UserParamIteratorFinished) (const AtUserParamIterator*) = nullptr;
            const char* (*UserParamGetName) (const AtUserParamEntry*) = nullptr;
            int (*UserParamGetType) (const AtUserParamEntry*) = nullptr;
            int (*UserParamGetArrayType) (const AtUserParamEntry* upentry) = nullptr;
            const char* (*NodeEntryGetName) (const AtNodeEntry* nentry) = nullptr;
            AtParamIterator* (*NodeEntryGetParamIterator) (const AtNodeEntry* nentry) = nullptr;
            void (*ParamIteratorDestroy) (AtParamIterator* iter) = nullptr;
            const AtParamEntry* (*ParamIteratorGetNext) (AtParamIterator* iter) = nullptr;
            bool (*ParamIteratorFinished) (const AtParamIterator* iter) = nullptr;
            const char* (*ParamGetName) (const AtParamEntry* pentry) = nullptr;
            int (*ParamGetType) (const AtParamEntry* pentry) = nullptr;
            int8_t (*NodeGetByte) (const AtNode*, const char*) = nullptr;
            int32_t (*NodeGetInt) (const AtNode*, const char*) = nullptr;
            uint32_t (*NodeGetUInt) (const AtNode*, const char*) = nullptr;
            bool (*NodeGetBool) (const AtNode*, const char*) = nullptr;
            float (*NodeGetFlt) (const AtNode*, const char*) = nullptr;
            GfVec3f (*NodeGetRGB) (const AtNode*, const char*) = nullptr;
            GfVec4f (*NodeGetRGBA) (const AtNode*, const char*) = nullptr;
            GfVec3f (*NodeGetVec) (const AtNode*, const char*) = nullptr;
            GfVec3f (*NodeGetPnt) (const AtNode*, const char*) = nullptr;
            GfVec2f (*NodeGetPnt2) (const AtNode*, const char*) = nullptr;
            const char* (*NodeGetStr) (const AtNode*, const char*) = nullptr;
            void* (*NodeGetPtr) (const AtNode*, const char*) = nullptr;
            AtArray* (*NodeGetArray) (const AtNode*, const char*) = nullptr;
            void (*_NodeGetMatrix) (const AtNode*, const char*, float*) = nullptr;

            GfMatrix4f NodeGetMatrix(const AtNode* node, const char* param) {
                GfMatrix4f ret;
                _NodeGetMatrix(node, param, ret[0]);
                return ret;
            };

            template <typename T>
            void convert_ptr(T& trg, void* sym) {
                trg = reinterpret_cast<T>(sym);
            }

            template <typename T>
            void load_ptr(T& trg, void* handle, const char* name) {
                auto tmp = dlsym(handle, name);
                if (tmp == nullptr) {
                    throw std::runtime_error(std::string("Error loading : ") + std::string(name));
                }
                convert_ptr(trg, tmp);
            }

            void* mtoa_handle = nullptr;
            void* ai_handle = nullptr;
            enum State {
                STATE_UNINITIALIZED,
                STATE_VALID,
                STATE_INVALID
            };
            State state = STATE_UNINITIALIZED;

            bool is_valid() {
                if (state == STATE_INVALID) {
                    return false;
                } else if (state == STATE_VALID) {
                    return true;
                }

                state = STATE_INVALID;
                auto ai_handle = dlopen("libai.so", RTLD_LAZY | RTLD_GLOBAL);
                if (ai_handle == nullptr) { return false; }
                std::string mtoa_path = std::string(getenv("MTOA_HOME")) + std::string("/plug-ins/mtoa.so");
                auto mtoa_handle = dlopen(mtoa_path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
                if (mtoa_handle == nullptr) { return false; }
                try {
                    load_ptr(mtoa_init_export_session, mtoa_handle, "mtoa_init_export_session");
                    load_ptr(mtoa_export_node, mtoa_handle, "mtoa_export_node");
                    load_ptr(mtoa_destroy_export_session, mtoa_handle, "mtoa_destroy_export_session");
                    load_ptr(NodeGetLink, ai_handle, "AiNodeGetLink");
                    load_ptr(NodeGetName, ai_handle, "AiNodeGetName");
                    load_ptr(NodeGetNodeEntry, ai_handle, "AiNodeGetNodeEntry");
                    load_ptr(NodeGetUserParamIterator, ai_handle, "AiNodeGetUserParamIterator");
                    load_ptr(UserParamIteratorDestroy, ai_handle, "AiUserParamIteratorDestroy");
                    load_ptr(UserParamIteratorGetNext, ai_handle, "AiUserParamIteratorGetNext");
                    load_ptr(UserParamIteratorFinished, ai_handle, "AiUserParamIteratorFinished");
                    load_ptr(UserParamGetName, ai_handle, "AiUserParamGetName");
                    load_ptr(UserParamGetType, ai_handle, "AiUserParamGetType");
                    load_ptr(UserParamGetArrayType, ai_handle, "AiUserParamGetArrayType");
                    load_ptr(NodeEntryGetName, ai_handle, "AiNodeEntryGetName");
                    load_ptr(NodeEntryGetParamIterator, ai_handle, "AiNodeEntryGetParamIterator");
                    load_ptr(ParamIteratorDestroy, ai_handle, "AiParamIteratorDestroy");
                    load_ptr(ParamIteratorGetNext, ai_handle, "AiParamIteratorGetNext");
                    load_ptr(ParamIteratorFinished, ai_handle, "AiParamIteratorFinished");
                    load_ptr(ParamGetName, ai_handle, "AiParamGetName");
                    load_ptr(ParamGetType, ai_handle, "AiParamGetType");
                    load_ptr(NodeGetByte, ai_handle, "AiNodeGetByte");
                    load_ptr(NodeGetInt, ai_handle, "AiNodeGetInt");
                    load_ptr(NodeGetUInt, ai_handle, "AiNodeGetUInt");
                    load_ptr(NodeGetBool, ai_handle, "AiNodeGetBool");
                    load_ptr(NodeGetFlt, ai_handle, "AiNodeGetFlt");
                    load_ptr(NodeGetRGB, ai_handle, "AiNodeGetRGB");
                    load_ptr(NodeGetRGBA, ai_handle, "AiNodeGetRGBA");
                    load_ptr(NodeGetVec, ai_handle, "AiNodeGetVec");
                    load_ptr(NodeGetPnt, ai_handle, "AiNodeGetPnt");
                    load_ptr(NodeGetPnt2, ai_handle, "AiNodeGetPnt2");
                    load_ptr(NodeGetStr, ai_handle, "AiNodeGetStr");
                    load_ptr(NodeGetPtr, ai_handle, "AiNodeGetPtr");
                    load_ptr(NodeGetArray, ai_handle, "AiNodeGetArray");
                    load_ptr(_NodeGetMatrix, ai_handle, "AiNodeGetMatrix");
                } catch (std::exception& e) {
                    std::cerr << e.what() << std::endl;
                    return false;
                }
                state = STATE_VALID;
                return true;
            }

            ~ArnoldCtx() {
                if (mtoa_handle != nullptr) {
                    dlclose(mtoa_handle);
                }
                if (ai_handle != nullptr) {
                    dlclose(ai_handle);
                }
            }
        };

        static ArnoldCtx ai;
    public:
        ArnoldShaderExport() {
            ai.mtoa_init_export_session();
        }

        ~ArnoldShaderExport() {
            ai.mtoa_destroy_export_session();
        }

        static bool is_valid() {
            return ai.is_valid();
        }

        SdfPath export_shader(const UsdStageRefPtr& stage, MObject obj, const char* attr) {
            const static SdfPath shaders_scope("/shaders");
            auto arnoldNode = ai.mtoa_export_node(&obj, attr);
            if (arnoldNode != nullptr) {
                auto shader_path = shaders_scope.AppendPath(SdfPath(ai.NodeGetName(arnoldNode)));
                auto shader = UsdAiShader::Define(stage, shader_path);
            } else {
                return SdfPath();
            }
        }
    };

    ArnoldShaderExport::ArnoldCtx ArnoldShaderExport::ai;
}
/* static */
void
PxrUsdMayaTranslatorLook::ExportShadingEngines(
        const UsdStageRefPtr& stage,
        const PxrUsdMayaUtil::ShapeSet& bindableRoots,
        const TfToken& shadingMode,
        bool mergeTransformAndShape,
        bool handleUsdNamespaces,
        SdfPath overrideRootPath)
{
    if (shadingMode == PxrUsdMayaShadingModeTokens->none) {
        return;
    }

    if (shadingMode == PxrUsdMayaShadingModeTokens->arnold && ArnoldShaderExport::is_valid()) {
        // traditional registry doesn't work here, as it's only a simple function pointer
        // which doesn't play well with starting up and shutting down the mtoa exporter

        const static SdfPath shaders_scope("/shaders");
        UsdGeomScope::Define(stage, shaders_scope);

        ArnoldShaderExport ai;
        // struct definitions
        for (MItDependencyNodes iter(MFn::kShadingEngine); !iter.isDone(); iter.next()) {
            MObject obj = iter.thisNode();
            const auto exportedShader = ai.export_shader(stage, obj, "message");
        }

    } else if (PxrUsdMayaShadingModeExporter exporter =
            PxrUsdMayaShadingModeRegistry::GetExporter(shadingMode)) {
        MItDependencyNodes shadingEngineIter(MFn::kShadingEngine);
        for (; !shadingEngineIter.isDone(); shadingEngineIter.next()) {
            MObject shadingEngine(shadingEngineIter.thisNode());
            MFnDependencyNode seDepNode(shadingEngine);

            PxrUsdMayaShadingModeExportContext c(
                    shadingEngine,
                    stage,
                    mergeTransformAndShape,
                    handleUsdNamespaces,
                    bindableRoots,
                    overrideRootPath);

            exporter(&c);
        }
    }
    else {
        MGlobal::displayError(TfStringPrintf("No shadingMode '%s' found.",
                    shadingMode.GetText()).c_str());
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

