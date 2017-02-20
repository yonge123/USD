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
#include "pxr/base/vt/array.h"

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

#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdAi/aiShader.h"
#include "pxr/usd/usdAi/aiMaterialAPI.h"
#include "pxr/usd/usdAi/aiNodeAPI.h"
#include "pxr/usd/sdf/types.h"

namespace {
    constexpr uint8_t AI_TYPE_BYTE = 0x00;
    constexpr uint8_t AI_TYPE_INT = 0x01;
    constexpr uint8_t AI_TYPE_UINT = 0x02;
    constexpr uint8_t AI_TYPE_BOOLEAN = 0x03;
    constexpr uint8_t AI_TYPE_FLOAT = 0x04;
    constexpr uint8_t AI_TYPE_RGB = 0x05;
    constexpr uint8_t AI_TYPE_RGBA = 0x06;
    constexpr uint8_t AI_TYPE_VECTOR = 0x07;
    constexpr uint8_t AI_TYPE_POINT = 0x08;
    constexpr uint8_t AI_TYPE_POINT2 = 0x09;
    constexpr uint8_t AI_TYPE_STRING = 0x0A;
    constexpr uint8_t AI_TYPE_POINTER = 0x0B;
    constexpr uint8_t AI_TYPE_NODE = 0x0C;
    constexpr uint8_t AI_TYPE_ARRAY = 0x0D;
    constexpr uint8_t AI_TYPE_MATRIX = 0x0E;
    constexpr uint8_t AI_TYPE_ENUM = 0x0F;
    constexpr uint8_t AI_TYPE_UNDEFINED = 0xFF;
    constexpr uint8_t AI_TYPE_NONE = 0xFF;

    class ArnoldShaderExport {
    private:
        struct AtNode;
        struct AtNodeEntry;
        struct AtUserParamIterator;
        struct AtUserParamEntry;
        struct AtParamIterator;
        struct AtParamEntry;
        struct AtArray {
            void* data;
            uint32_t nelements;
            uint8_t nkeys;
            uint8_t type;
        };
        struct AtRGB {
            float r, g, b;

            operator GfVec3f() { return GfVec3f(r, g, b); }
        };
        struct AtRGBA {
            float r, g, b, a;

            operator GfVec4f() { return GfVec4f(r, g, b, a); }
        };
        struct AtPnt {
            float x, y, z;

            operator GfVec3f() { return GfVec3f(x, y, z); }
        };
        struct AtPnt2 {
            float x, y;

            operator GfVec2f() { return GfVec2f(x, y); }
        };

        struct ArnoldCtx {
            // TODO: rewrite to std::function and use static_cast / reinterpret_cast
            // mtoa functions
            void (*mtoa_init_export_session) () = nullptr;
            AtNode* (*mtoa_export_node) (void*, const char*) = nullptr;
            void (*mtoa_destroy_export_session) () = nullptr;
            // Node functions
            AtNode* (*NodeGetLink) (const AtNode*, const char*, int32_t*) = nullptr;
            const char* (*NodeGetName) (const AtNode*) = nullptr;
            const AtNodeEntry* (*NodeGetNodeEntry) (const AtNode*) = nullptr;
            AtUserParamIterator* (*NodeGetUserParamIterator) (const AtNode*) = nullptr;
            // User entry functions
            void (*UserParamIteratorDestroy) (AtUserParamIterator*) = nullptr;
            const AtUserParamEntry* (*UserParamIteratorGetNext) (AtUserParamIterator*) = nullptr;
            bool (*UserParamIteratorFinished) (const AtUserParamIterator*) = nullptr;
            const char* (*UserParamGetName) (const AtUserParamEntry*) = nullptr;
            int (*UserParamGetType) (const AtUserParamEntry*) = nullptr;
            int (*UserParamGetArrayType) (const AtUserParamEntry*) = nullptr;
            // Node entry functions
            const char* (*NodeEntryGetName) (const AtNodeEntry*) = nullptr;
            AtParamIterator* (*NodeEntryGetParamIterator) (const AtNodeEntry*) = nullptr;
            int32_t (*NodeEntryGetOutputType) (const AtNodeEntry*) = nullptr;
            // Param functions
            void (*ParamIteratorDestroy) (AtParamIterator*) = nullptr;
            const AtParamEntry* (*ParamIteratorGetNext) (AtParamIterator*) = nullptr;
            bool (*ParamIteratorFinished) (const AtParamIterator*) = nullptr;
            const char* (*ParamGetName) (const AtParamEntry*) = nullptr;
            int (*ParamGetType) (const AtParamEntry*) = nullptr;
            // Node functions
            uint8_t (*NodeGetByte) (const AtNode*, const char*) = nullptr;
            int32_t (*NodeGetInt) (const AtNode*, const char*) = nullptr;
            uint32_t (*NodeGetUInt) (const AtNode*, const char*) = nullptr;
            bool (*NodeGetBool) (const AtNode*, const char*) = nullptr;
            float (*NodeGetFlt) (const AtNode*, const char*) = nullptr;
            AtRGB (*NodeGetRGB) (const AtNode*, const char*) = nullptr;
            AtRGBA (*NodeGetRGBA) (const AtNode*, const char*) = nullptr;
            AtPnt (*NodeGetVec) (const AtNode*, const char*) = nullptr;
            AtPnt (*NodeGetPnt) (const AtNode*, const char*) = nullptr;
            AtPnt2 (*NodeGetPnt2) (const AtNode*, const char*) = nullptr;
            const char* (*NodeGetStr) (const AtNode*, const char*) = nullptr;
            void* (*NodeGetPtr) (const AtNode*, const char*) = nullptr;
            AtArray* (*NodeGetArray) (const AtNode*, const char*) = nullptr;
            void (*_NodeGetMatrix) (const AtNode*, const char*, float*) = nullptr;
            // Array functions
            bool (*ArrayGetBoolFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            uint8_t (*ArrayGetByteFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            int32_t (*ArrayGetIntFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            uint32_t (*ArrayGetUIntFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            float (*ArrayGetFltFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            AtRGB (*ArrayGetRGBFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            AtRGBA (*ArrayGetRGBAFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            AtPnt (*ArrayGetPntFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            AtPnt2 (*ArrayGetPnt2Func) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            AtPnt (*ArrayGetVecFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            void (*ArrayGetMtxFunc) (const AtArray*, uint32_t, float*, const char*, uint32_t) = nullptr;
            const char* (*ArrayGetStrFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            void* (*ArrayGetPtrFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;
            AtArray* (*ArrayGetArrayFunc) (const AtArray*, uint32_t, const char*, uint32_t) = nullptr;

            inline GfMatrix4f NodeGetMatrix(const AtNode* node, const char* param) {
                GfMatrix4f ret;
                _NodeGetMatrix(node, param, ret[0]);
                return ret;
            };

            inline GfMatrix4f ArrayGetMatrix(const AtArray* arr, uint32_t i, const char* file, uint32_t line) {
                GfMatrix4f ret;
                ArrayGetMtxFunc(arr, i, ret[0], file, line);
                return ret;
            }

            void* mtoa_handle = nullptr;
            void* ai_handle = nullptr;
            enum State {
                STATE_UNINITIALIZED,
                STATE_VALID,
                STATE_INVALID
            };
            State state = STATE_UNINITIALIZED;

            template <typename T>
            void convert_ptr(T& trg, void* sym) {
                trg = reinterpret_cast<T>(sym);
            }

            template <typename T>
            void mtoa_ptr(T& trg, const char* name) {
                auto tmp = dlsym(mtoa_handle, name);
                if (tmp == nullptr) {
                    throw std::runtime_error(std::string("Error loading : ") + std::string(name));
                }
                convert_ptr(trg, tmp);
            }

            template <typename T>
            void ai_ptr(T& trg, const char* name) {
                auto tmp = dlsym(ai_handle, name);
                if (tmp == nullptr) {
                    throw std::runtime_error(std::string("Error loading : ") + std::string(name));
                }
                convert_ptr(trg, tmp);
            }

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
                    mtoa_ptr(mtoa_init_export_session, "mtoa_init_export_session");
                    mtoa_ptr(mtoa_export_node, "mtoa_export_node");
                    mtoa_ptr(mtoa_destroy_export_session, "mtoa_destroy_export_session");
                    ai_ptr(NodeGetLink, "AiNodeGetLink");
                    ai_ptr(NodeGetName, "AiNodeGetName");
                    ai_ptr(NodeGetNodeEntry, "AiNodeGetNodeEntry");
                    ai_ptr(NodeGetUserParamIterator, "AiNodeGetUserParamIterator");
                    ai_ptr(UserParamIteratorDestroy, "AiUserParamIteratorDestroy");
                    ai_ptr(UserParamIteratorGetNext, "AiUserParamIteratorGetNext");
                    ai_ptr(UserParamIteratorFinished, "AiUserParamIteratorFinished");
                    ai_ptr(UserParamGetName, "AiUserParamGetName");
                    ai_ptr(UserParamGetType, "AiUserParamGetType");
                    ai_ptr(UserParamGetArrayType, "AiUserParamGetArrayType");
                    ai_ptr(NodeEntryGetName, "AiNodeEntryGetName");
                    ai_ptr(NodeEntryGetParamIterator, "AiNodeEntryGetParamIterator");
                    ai_ptr(NodeEntryGetOutputType, "AiNodeEntryGetOutputType");
                    ai_ptr(ParamIteratorDestroy, "AiParamIteratorDestroy");
                    ai_ptr(ParamIteratorGetNext, "AiParamIteratorGetNext");
                    ai_ptr(ParamIteratorFinished, "AiParamIteratorFinished");
                    ai_ptr(ParamGetName, "AiParamGetName");
                    ai_ptr(ParamGetType, "AiParamGetType");
                    ai_ptr(NodeGetByte, "AiNodeGetByte");
                    ai_ptr(NodeGetInt, "AiNodeGetInt");
                    ai_ptr(NodeGetUInt, "AiNodeGetUInt");
                    ai_ptr(NodeGetBool, "AiNodeGetBool");
                    ai_ptr(NodeGetFlt, "AiNodeGetFlt");
                    ai_ptr(NodeGetRGB, "AiNodeGetRGB");
                    ai_ptr(NodeGetRGBA, "AiNodeGetRGBA");
                    ai_ptr(NodeGetVec, "AiNodeGetVec");
                    ai_ptr(NodeGetPnt, "AiNodeGetPnt");
                    ai_ptr(NodeGetPnt2, "AiNodeGetPnt2");
                    ai_ptr(NodeGetStr, "AiNodeGetStr");
                    ai_ptr(NodeGetPtr, "AiNodeGetPtr");
                    ai_ptr(NodeGetArray, "AiNodeGetArray");
                    ai_ptr(_NodeGetMatrix, "AiNodeGetMatrix");
                    ai_ptr(ArrayGetBoolFunc, "AiArrayGetBoolFunc");
                    ai_ptr(ArrayGetByteFunc, "AiArrayGetByteFunc");
                    ai_ptr(ArrayGetIntFunc, "AiArrayGetIntFunc");
                    ai_ptr(ArrayGetUIntFunc, "AiArrayGetUIntFunc");
                    ai_ptr(ArrayGetFltFunc, "AiArrayGetFltFunc");
                    ai_ptr(ArrayGetRGBFunc, "AiArrayGetRGBFunc");
                    ai_ptr(ArrayGetRGBAFunc, "AiArrayGetRGBAFunc");
                    ai_ptr(ArrayGetPntFunc, "AiArrayGetPntFunc");
                    ai_ptr(ArrayGetPnt2Func, "AiArrayGetPnt2Func");
                    ai_ptr(ArrayGetVecFunc, "AiArrayGetVecFunc");
                    ai_ptr(ArrayGetMtxFunc, "AiArrayGetMtxFunc");
                    ai_ptr(ArrayGetStrFunc, "AiArrayGetStrFunc");
                    ai_ptr(ArrayGetPtrFunc, "AiArrayGetPtrFunc");
                    ai_ptr(ArrayGetArrayFunc, "AiArrayGetArrayFunc");
                } catch (std::exception& e) {
                    std::cerr << e.what() << std::endl;
                    return false;
                }
                state = STATE_VALID;
                return true;
            }

            ArnoldCtx() {

            }

            ~ArnoldCtx() {
                if (mtoa_handle != nullptr) {
                    dlclose(mtoa_handle);
                }
                if (ai_handle != nullptr) {
                    dlclose(ai_handle);
                }
            }

            // Preferring std::vector here over pointers, so we can get some extra
            // boundary checks in debug mode.
            static
            const TfToken& out_comp_name(int32_t output_type, int32_t index) {
                const auto i = std::min(std::max(index + 1, 0), 4);
                const static std::array<TfToken, 5> rgb_comp_names = {
                    TfToken("node"), TfToken("r"), TfToken("g"), TfToken("b"), TfToken("a")};
                const static std::array<TfToken, 5> vec_comp_names = {
                    TfToken("node"), TfToken("x"), TfToken("y"), TfToken("z"), TfToken("w")};
                const static std::array<TfToken, 5> other_comp_names = {
                    TfToken("node"), TfToken("node"), TfToken("node"), TfToken("node"), TfToken("node")};
                switch (output_type) {
                    case AI_TYPE_RGB: case AI_TYPE_RGBA:
                        return rgb_comp_names[i];
                    case AI_TYPE_VECTOR: case AI_TYPE_POINT: case AI_TYPE_POINT2:
                        return vec_comp_names[i];
                    default:
                        return other_comp_names[i];
                }
            }
        };

        static ArnoldCtx ai;
    private:
        // TODO: use sorted vectors and arrays with lower bound lookups
        struct simple_type_map_elem {
            const SdfValueTypeName& type;
            std::function<VtValue(const AtNode*, const char*)> f;
        };
        std::map<uint8_t, simple_type_map_elem> simple_type_map;

        template <typename T, typename R = T> inline
        void export_array(UsdShadeParameter& param, const AtArray* arr, R (*f) (const AtArray*, uint32_t, const char*, uint32_t)) {
            // we already check the validity of the array before this call
            VtArray<T> out_arr(arr->nelements);
            for (auto i = 0u; i < arr->nelements; ++i) {
                out_arr[i] = f(arr, i, __FILE__, __LINE__);
            }
            param.Set(VtValue(out_arr));
        }

        struct array_type_map_elem {
            const SdfValueTypeName& type;
            std::function<void(UsdShadeParameter&, const AtArray*)> f;

            array_type_map_elem(const SdfValueTypeName& _type,
                                std::function<void(UsdShadeParameter&, const AtArray*)> _f) :
                type(_type), f(_f) { }
        };
        std::map<uint8_t, array_type_map_elem> array_type_map;

    public:
        ArnoldShaderExport(const UsdStageRefPtr& _stage, UsdTimeCode _time_code) :
            m_stage(_stage), m_shaders_scope("/shaders"), m_time_code(_time_code) {
            UsdGeomScope::Define(m_stage, m_shaders_scope);
            ai.mtoa_init_export_session();

            simple_type_map = {
                {AI_TYPE_BYTE, {SdfValueTypeNames.Get()->UChar, [this](const AtNode* no, const char* na) -> VtValue { return VtValue(this->ai.NodeGetByte(no, na)); }}},
                {AI_TYPE_INT, {SdfValueTypeNames.Get()->Int, [this](const AtNode* no, const char* na) -> VtValue { return VtValue(this->ai.NodeGetInt(no, na)); }}},
                {AI_TYPE_UINT, {SdfValueTypeNames.Get()->UInt, [this](const AtNode* no, const char* na) -> VtValue { return VtValue(this->ai.NodeGetUInt(no, na)); }}},
                {AI_TYPE_BOOLEAN, {SdfValueTypeNames.Get()->Bool, [this](const AtNode* no, const char* na) -> VtValue { return VtValue(this->ai.NodeGetBool(no, na)); }}},
                {AI_TYPE_FLOAT, {SdfValueTypeNames.Get()->Float, [this](const AtNode* no, const char* na) -> VtValue { return VtValue(this->ai.NodeGetFlt(no, na)); }}},
                {AI_TYPE_RGB, {SdfValueTypeNames.Get()->Color3f, [this](const AtNode* no, const char* na) -> VtValue { auto v = this->ai.NodeGetRGB(no, na); return VtValue(GfVec3f(v.r, v.g, v.b)); }}},
                {AI_TYPE_RGBA, {SdfValueTypeNames.Get()->Color4f, [this](const AtNode* no, const char* na) -> VtValue { auto v = this->ai.NodeGetRGBA(no, na); return VtValue(GfVec4f(v.r, v.g, v.b, v.a)); }}},
                {AI_TYPE_VECTOR, {SdfValueTypeNames.Get()->Vector3f, [this](const AtNode* no, const char* na) -> VtValue { auto v = this->ai.NodeGetVec(no, na); return VtValue(GfVec3f(v.x, v.y, v.z)); }}},
                {AI_TYPE_POINT, {SdfValueTypeNames.Get()->Vector3f, [this](const AtNode* no, const char* na) -> VtValue { auto v = this->ai.NodeGetPnt(no, na); return VtValue(GfVec3f(v.x, v.y, v.z)); }}},
                {AI_TYPE_POINT2, {SdfValueTypeNames.Get()->Float2, [this](const AtNode* no, const char* na) -> VtValue { auto v = this->ai.NodeGetPnt2(no, na); return VtValue(GfVec2f(v.x, v.y)); }}},
                {AI_TYPE_STRING, {SdfValueTypeNames.Get()->String, [this](const AtNode* no, const char* na) -> VtValue { return VtValue(this->ai.NodeGetStr(no, na)); }}},
                {AI_TYPE_NODE,
                 {SdfValueTypeNames.Get()->String,
                  [this](const AtNode* no, const char* na) -> VtValue
                  {
                      return VtValue(write_arnold_node(reinterpret_cast<const AtNode*>(ai.NodeGetPtr(no, na))).GetPath().GetString());
                  }}}, // TODO: Do connections here?
                {AI_TYPE_MATRIX, {SdfValueTypeNames.Get()->Matrix4d, [this](const AtNode* no, const char* na) -> VtValue { return VtValue(this->ai.NodeGetMatrix(no, na)); }}},
                {AI_TYPE_ENUM, {SdfValueTypeNames.Get()->Int, [this](const AtNode* no, const char* na) -> VtValue { return VtValue(this->ai.NodeGetInt(no, na)); }}},
            };

            array_type_map = {
                {AI_TYPE_BYTE, {SdfValueTypeNames.Get()->UCharArray, [this](UsdShadeParameter& p, const AtArray* a) { export_array<uint8_t>(p, a, this->ai.ArrayGetByteFunc); }}},
                {AI_TYPE_INT, {SdfValueTypeNames.Get()->IntArray, [this](UsdShadeParameter& p, const AtArray* a) { export_array<int32_t>(p, a, this->ai.ArrayGetIntFunc); }}},
                {AI_TYPE_UINT, {SdfValueTypeNames.Get()->UIntArray, [this](UsdShadeParameter& p, const AtArray* a) { export_array<uint32_t>(p, a, this->ai.ArrayGetUIntFunc); }}},
                {AI_TYPE_BOOLEAN, {SdfValueTypeNames.Get()->BoolArray, [this](UsdShadeParameter& p, const AtArray* a) { export_array<bool>(p, a, this->ai.ArrayGetBoolFunc); }}},
                {AI_TYPE_FLOAT, {SdfValueTypeNames.Get()->FloatArray, [this](UsdShadeParameter& p, const AtArray* a) { export_array<float>(p, a, this->ai.ArrayGetFltFunc); }}},
                {AI_TYPE_RGB, {SdfValueTypeNames.Get()->Color3fArray, [this](UsdShadeParameter& p, const AtArray* a) { export_array<GfVec3f, AtRGB>(p, a, this->ai.ArrayGetRGBFunc); }}},
                {AI_TYPE_RGBA, {SdfValueTypeNames.Get()->Color4fArray, [this](UsdShadeParameter& p, const AtArray* a) { export_array<GfVec4f, AtRGBA>(p, a, this->ai.ArrayGetRGBAFunc); }}},
                {AI_TYPE_VECTOR, {SdfValueTypeNames.Get()->Vector3fArray, [this](UsdShadeParameter& p, const AtArray* a) { export_array<GfVec3f, AtPnt>(p, a, this->ai.ArrayGetVecFunc); }}},
                {AI_TYPE_POINT, {SdfValueTypeNames.Get()->Vector3fArray, [this](UsdShadeParameter& p, const AtArray* a) { export_array<GfVec3f, AtPnt>(p, a, this->ai.ArrayGetPntFunc); }}},
                {AI_TYPE_POINT2, {SdfValueTypeNames.Get()->Float2Array, [this](UsdShadeParameter& p, const AtArray* a) { export_array<GfVec2f, AtPnt2>(p, a, this->ai.ArrayGetPnt2Func); }}},
                {AI_TYPE_STRING, {SdfValueTypeNames.Get()->StringArray, [this](UsdShadeParameter& p, const AtArray* a) { export_array<std::string, const char*>(p, a, this->ai.ArrayGetStrFunc); }}},
                {AI_TYPE_NODE,
                 {SdfValueTypeNames.Get()->StringArray,
                  [this](UsdShadeParameter& p, const AtArray* a)
                  {
                      VtArray<std::string> arr(a->nelements);
                      for (auto i = 0u; i < a->nelements; ++i) {
                          arr[i] = write_arnold_node(reinterpret_cast<const AtNode*>(ai.ArrayGetPtrFunc(a, i, __FILE__, __LINE__))).GetPath().GetString();
                      }
                  }}},
                {AI_TYPE_MATRIX,
                 {SdfValueTypeNames.Get()->Matrix4dArray,
                  [this](UsdShadeParameter& p, const AtArray* a) {
                      VtArray<GfMatrix4d> arr(a->nelements);
                      for (auto i = 0u; i < a->nelements; ++i) {
                          arr[i] = GfMatrix4d(this->ai.ArrayGetMatrix(a, i, __FILE__, __LINE__));
                      }
                  }
                 }}, // TODO: implement
                {AI_TYPE_ENUM, {SdfValueTypeNames.Get()->IntArray, [this](UsdShadeParameter& p, const AtArray* a) { export_array<int32_t>(p, a, this->ai.ArrayGetIntFunc); }}},
            };
        }

        ~ArnoldShaderExport() {
            ai.mtoa_destroy_export_session();
        }

        static bool is_valid() {
            return ai.is_valid();
        }

    private:
        std::map<const AtNode*, UsdPrim> m_shader_to_usd_path;
        const UsdStageRefPtr& m_stage;
        SdfPath m_shaders_scope;
        UsdTimeCode m_time_code;

        inline void export_parameter(const AtNode* arnold_node, UsdAiShader& shader, const char* pname, uint8_t ptype, bool user) {
            if (ptype == AI_TYPE_ARRAY) {
                const auto arr = ai.NodeGetArray(arnold_node, pname);
                if (arr == nullptr || arr->nelements == 0 || arr->nkeys == 0 || arr->type == AI_TYPE_ARRAY) { return; }
                const auto itype = array_type_map.find(arr->type);
                if (itype == array_type_map.end() || itype->second.f == nullptr) { return; }
                auto param = shader.CreateParameter(TfToken(pname), itype->second.type);
                itype->second.f(param, arr);
            } else {
                const auto itype = simple_type_map.find(ptype);
                if (itype == simple_type_map.end()) { return; }
                if (user) {
                    UsdAiNodeAPI api(shader.GetPrim());
                    api.CreateUserAttribute(TfToken(pname), itype->second.type);
                } else {
                    auto param = shader.CreateParameter(TfToken(pname), itype->second.type);
                    // Connections to user parameters don't make much sense.
                    if (!param || user) { return; }
                    // These checks can't be easily moved out, or we'll just put complexity somewhere else.
                    // Accessing AtParam directly won't give us the "type" checks the arnold functions are doing.
                    param.Set(itype->second.f(arnold_node, pname));
                    int32_t comp = -1;
                    const auto linked_node = ai.NodeGetLink(arnold_node, pname, &comp);
                    if (linked_node != nullptr) {
                        const auto linked_prim = write_arnold_node(linked_node);
                        if (!linked_prim.IsValid()) { return; }
                        UsdShadeShader linked_shader(linked_prim);
                        // We make sure there are 5 elems in each array, thus the w and multiple nodes
                        // so there is no need for extra checks.

                        const auto linked_output_type = ai.NodeEntryGetOutputType(ai.NodeGetNodeEntry(linked_node));
                        const auto& out_name = ArnoldCtx::out_comp_name(linked_output_type, comp);
                        auto out_param = linked_shader.GetOutput(out_name);
                        if (out_param) {
                            param.ConnectToSource(out_param);
                        } else {
                            // The automatic conversion will be handled by arnold, so make the full connection
                            // something neutral, like Token. Otherwise it's always float, there are no
                            // multicomponent, no float types in arnold.
                            param.ConnectToSource(
                                linked_shader.CreateOutput(out_name,
                                                           out_name == "node" ?
                                                           SdfValueTypeNames.Get()->Float :
                                                           SdfValueTypeNames.Get()->Token));
                        }
                    }
                }
            }
        }

        UsdPrim write_arnold_node(const AtNode* arnold_node) {
            if (arnold_node != nullptr) {
                const auto it = m_shader_to_usd_path.find(arnold_node);
                if (it != m_shader_to_usd_path.end()) {
                    return it->second;
                } else {
                    std::string arnold_name_cleanup(ai.NodeGetName(arnold_node));
                    // MtoA exports sub shaders with @ prefix, which is used for something else in USD
                    // TODO: implement a proper cleanup using boost::regex
                    std::replace(arnold_name_cleanup.begin(), arnold_name_cleanup.end(), '@', '_');
                    auto shader_path = m_shaders_scope.AppendPath(SdfPath(arnold_name_cleanup));
                    auto shader = UsdAiShader::Define(m_stage, shader_path);
                    m_shader_to_usd_path.insert(std::make_pair(arnold_node, shader.GetPrim()));

                    const auto nentry = ai.NodeGetNodeEntry(arnold_node);
                    shader.CreateIdAttr(VtValue(TfToken(ai.NodeEntryGetName(nentry))));
                    auto piter = ai.NodeEntryGetParamIterator(nentry);
                    while (!ai.ParamIteratorFinished(piter)) {
                        const auto pentry = ai.ParamIteratorGetNext(piter);
                        auto pname = ai.ParamGetName(pentry);
                        if (strcmp(pname, "name") == 0) { continue; }
                        const auto ptype = static_cast<uint8_t>(ai.ParamGetType(pentry));
                        export_parameter(arnold_node, shader, pname, ptype, false);
                    }
                    ai.ParamIteratorDestroy(piter);
                    auto puiter = ai.NodeGetUserParamIterator(arnold_node);
                    while (!ai.UserParamIteratorFinished(puiter)) {
                        const auto pentry = ai.UserParamIteratorGetNext(puiter);
                        auto pname = ai.UserParamGetName(pentry);
                        const auto ptype = static_cast<uint8_t>(ai.UserParamGetType(pentry));
                        export_parameter(arnold_node, shader, pname, ptype, true);
                    }
                    ai.UserParamIteratorDestroy(puiter);
                    return shader.GetPrim();
                }
            } else {
                return UsdPrim();
            }
        }
    public:
        UsdPrim export_shader(MObject obj, const char* attr) {
            auto arnold_node = ai.mtoa_export_node(&obj, attr);
            return write_arnold_node(arnold_node);
        }
    };

    ArnoldShaderExport::ArnoldCtx ArnoldShaderExport::ai;
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

    if (shadingMode == PxrUsdMayaShadingModeTokens->arnold && ArnoldShaderExport::is_valid()) {
        // traditional registry doesn't work here, as it's only a simple function pointer
        // which doesn't play well with starting up and shutting down the mtoa exporter
        ArnoldShaderExport ai(stage, UsdTimeCode::Default());
        // struct definitions
        for (MItDependencyNodes iter(MFn::kShadingEngine); !iter.isDone(); iter.next()) {
            MObject obj = iter.thisNode();
            const auto exportedShader = ai.export_shader(obj, "message");
        }
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

