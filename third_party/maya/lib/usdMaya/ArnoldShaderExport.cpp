#include "usdMaya/ArnoldShaderExport.h"

#include "pxr/base/gf/matrix4f.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usdAi/aiMaterialAPI.h"
#include "pxr/usd/usdAi/aiNodeAPI.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/material.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

#include <dlfcn.h>

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

    constexpr uint32_t AI_NODE_SHADER = 0x0010;


    struct AtNodeEntry;
    struct AtUserParamIterator;
    struct AtUserParamEntry;
    struct AtParamIterator;
    struct AtParamEntry;
    struct AtArray;
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
    struct AtArray {
        void* data;
        uint32_t nelements;
        uint8_t nkeys;
        uint8_t type;
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
        int32_t (*NodeEntryGetType) (const AtNodeEntry*) = nullptr;
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
                ai_ptr(NodeEntryGetType, "AiNodeEntryGetType");
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
    };

    ArnoldCtx ai;

    template <typename T, typename R = T> inline
    void export_array(UsdShadeParameter& param, const AtArray* arr, R (*f) (const AtArray*, uint32_t, const char*, uint32_t)) {
        // we already check the validity of the array before this call
        VtArray<T> out_arr(arr->nelements);
        for (auto i = 0u; i < arr->nelements; ++i) {
            out_arr[i] = f(arr, i, __FILE__, __LINE__);
        }
        param.Set(VtValue(out_arr));
    }

    struct simple_type {
        const SdfValueTypeName& type;
        std::function<VtValue(const AtNode*, const char*)> f;

        simple_type(const SdfValueTypeName& _type, std::function<VtValue(const AtNode*, const char*)> _f) :
            type(_type), f(_f) { }
    };

    // TODO: use sorted vectors with lower bound searches. Too many ptr jumps here
    const std::map<uint8_t, simple_type> simple_type_map = {
        {AI_TYPE_BYTE, {SdfValueTypeNames.Get()->UChar, [](const AtNode* no, const char* na) -> VtValue { return VtValue(ai.NodeGetByte(no, na)); }}},
        {AI_TYPE_INT, {SdfValueTypeNames.Get()->Int, [](const AtNode* no, const char* na) -> VtValue { return VtValue(ai.NodeGetInt(no, na)); }}},
        {AI_TYPE_UINT, {SdfValueTypeNames.Get()->UInt, [](const AtNode* no, const char* na) -> VtValue { return VtValue(ai.NodeGetUInt(no, na)); }}},
        {AI_TYPE_BOOLEAN, {SdfValueTypeNames.Get()->Bool, [](const AtNode* no, const char* na) -> VtValue { return VtValue(ai.NodeGetBool(no, na)); }}},
        {AI_TYPE_FLOAT, {SdfValueTypeNames.Get()->Float, [](const AtNode* no, const char* na) -> VtValue { return VtValue(ai.NodeGetFlt(no, na)); }}},
        {AI_TYPE_RGB, {SdfValueTypeNames.Get()->Color3f, [](const AtNode* no, const char* na) -> VtValue { auto v = ai.NodeGetRGB(no, na); return VtValue(GfVec3f(v.r, v.g, v.b)); }}},
        {AI_TYPE_RGBA, {SdfValueTypeNames.Get()->Color4f, [](const AtNode* no, const char* na) -> VtValue { auto v = ai.NodeGetRGBA(no, na); return VtValue(GfVec4f(v.r, v.g, v.b, v.a)); }}},
        {AI_TYPE_VECTOR, {SdfValueTypeNames.Get()->Vector3f, [](const AtNode* no, const char* na) -> VtValue { auto v = ai.NodeGetVec(no, na); return VtValue(GfVec3f(v.x, v.y, v.z)); }}},
        {AI_TYPE_POINT, {SdfValueTypeNames.Get()->Vector3f, [](const AtNode* no, const char* na) -> VtValue { auto v = ai.NodeGetPnt(no, na); return VtValue(GfVec3f(v.x, v.y, v.z)); }}},
        {AI_TYPE_POINT2, {SdfValueTypeNames.Get()->Float2, [](const AtNode* no, const char* na) -> VtValue { auto v = ai.NodeGetPnt2(no, na); return VtValue(GfVec2f(v.x, v.y)); }}},
        {AI_TYPE_STRING, {SdfValueTypeNames.Get()->String, [](const AtNode* no, const char* na) -> VtValue { return VtValue(ai.NodeGetStr(no, na)); }}},
        {AI_TYPE_NODE, {SdfValueTypeNames.Get()->String, nullptr}},
        {AI_TYPE_MATRIX, {SdfValueTypeNames.Get()->Matrix4d, [](const AtNode* no, const char* na) -> VtValue { return VtValue(ai.NodeGetMatrix(no, na)); }}},
        {AI_TYPE_ENUM, {SdfValueTypeNames.Get()->Int, [](const AtNode* no, const char* na) -> VtValue { return VtValue(ai.NodeGetInt(no, na)); }}},
    };

    const simple_type*
    get_simple_type(uint8_t type) {
        const auto it = simple_type_map.find(type);
        if (it != simple_type_map.end()) {
            return &it->second;
        } else {
            return nullptr;
        }
    }

    struct array_type {
        const SdfValueTypeName& type;
        std::function<void(UsdShadeParameter&, const AtArray*)> f;

        array_type(const SdfValueTypeName& _type, std::function<void(UsdShadeParameter&, const AtArray*)> _f) :
            type(_type), f(_f) { }
    };

    const std::map<uint8_t, array_type> array_type_map = {
        {AI_TYPE_BYTE, {SdfValueTypeNames.Get()->UCharArray, [](UsdShadeParameter& p, const AtArray* a) { export_array<uint8_t>(p, a, ai.ArrayGetByteFunc); }}},
        {AI_TYPE_INT, {SdfValueTypeNames.Get()->IntArray, [](UsdShadeParameter& p, const AtArray* a) { export_array<int32_t>(p, a, ai.ArrayGetIntFunc); }}},
        {AI_TYPE_UINT, {SdfValueTypeNames.Get()->UIntArray, [](UsdShadeParameter& p, const AtArray* a) { export_array<uint32_t>(p, a, ai.ArrayGetUIntFunc); }}},
        {AI_TYPE_BOOLEAN, {SdfValueTypeNames.Get()->BoolArray, [](UsdShadeParameter& p, const AtArray* a) { export_array<bool>(p, a, ai.ArrayGetBoolFunc); }}},
        {AI_TYPE_FLOAT, {SdfValueTypeNames.Get()->FloatArray, [](UsdShadeParameter& p, const AtArray* a) { export_array<float>(p, a, ai.ArrayGetFltFunc); }}},
        {AI_TYPE_RGB, {SdfValueTypeNames.Get()->Color3fArray, [](UsdShadeParameter& p, const AtArray* a) { export_array<GfVec3f, AtRGB>(p, a, ai.ArrayGetRGBFunc); }}},
        {AI_TYPE_RGBA, {SdfValueTypeNames.Get()->Color4fArray, [](UsdShadeParameter& p, const AtArray* a) { export_array<GfVec4f, AtRGBA>(p, a, ai.ArrayGetRGBAFunc); }}},
        {AI_TYPE_VECTOR, {SdfValueTypeNames.Get()->Vector3fArray, [](UsdShadeParameter& p, const AtArray* a) { export_array<GfVec3f, AtPnt>(p, a, ai.ArrayGetVecFunc); }}},
        {AI_TYPE_POINT, {SdfValueTypeNames.Get()->Vector3fArray, [](UsdShadeParameter& p, const AtArray* a) { export_array<GfVec3f, AtPnt>(p, a, ai.ArrayGetPntFunc); }}},
        {AI_TYPE_POINT2, {SdfValueTypeNames.Get()->Float2Array, [](UsdShadeParameter& p, const AtArray* a) { export_array<GfVec2f, AtPnt2>(p, a, ai.ArrayGetPnt2Func); }}},
        {AI_TYPE_STRING, {SdfValueTypeNames.Get()->StringArray, [](UsdShadeParameter& p, const AtArray* a) { export_array<std::string, const char*>(p, a, ai.ArrayGetStrFunc); }}},
        {AI_TYPE_NODE, {SdfValueTypeNames.Get()->StringArray, nullptr}},
        {AI_TYPE_MATRIX,
            {SdfValueTypeNames.Get()->Matrix4dArray,
                           [](UsdShadeParameter& p, const AtArray* a) {
                               VtArray<GfMatrix4d> arr(a->nelements);
                               for (auto i = 0u; i < a->nelements; ++i) {
                                   arr[i] = GfMatrix4d(ai.ArrayGetMatrix(a, i, __FILE__, __LINE__));
                               }
                           }
                       }}, // TODO: implement
        {AI_TYPE_ENUM, {SdfValueTypeNames.Get()->IntArray, [](UsdShadeParameter& p, const AtArray* a) { export_array<int32_t>(p, a, ai.ArrayGetIntFunc); }}},
    };

    const array_type*
    get_array_type(uint8_t type) {
        const auto it = array_type_map.find(type);
        if (it != array_type_map.end()) {
            return &it->second;
        } else {
            return nullptr;
        }
    }

    struct out_comp_t {
        TfToken n;
        const SdfValueTypeName& t;

        out_comp_t(const char* _n, const SdfValueTypeName& _t) : n(_n), t(_t) { }
    };
    // Preferring std::vector here over pointers, so we can get some extra
    // boundary checks in debug mode.
    const out_comp_t out_comp_name(int32_t output_type, int32_t index) {
        const static out_comp_t node_comp_name = {"node", SdfValueTypeNames.Get()->String};
        const static std::vector<out_comp_t> rgba_comp_names = {
            {"r", SdfValueTypeNames.Get()->Float},
            {"g", SdfValueTypeNames.Get()->Float},
            {"b", SdfValueTypeNames.Get()->Float},
            {"a", SdfValueTypeNames.Get()->Float},
        };
        const static std::vector<out_comp_t> vec_comp_names = {
            {"x", SdfValueTypeNames.Get()->Float},
            {"t", SdfValueTypeNames.Get()->Float},
            {"z", SdfValueTypeNames.Get()->Float},
        };
        if (index == -1) {
            auto itype = get_simple_type(static_cast<uint8_t>(output_type));
            if (itype == nullptr) {
                return node_comp_name;
            } else {
                return {"out", itype->type};
            }
        } else {
            if (output_type == AI_TYPE_RGBA || output_type == AI_TYPE_RGB) {
                return rgba_comp_names[std::min(static_cast<size_t>(index), rgba_comp_names.size() - 1)];
            } else if (output_type == AI_TYPE_VECTOR || output_type == AI_TYPE_POINT ||
                       output_type == AI_TYPE_POINT2) {
                return vec_comp_names[std::min(static_cast<size_t>(index), vec_comp_names.size() - 1)];
            } else {
                return node_comp_name;
            }
        }
    }
}

ArnoldShaderExport::ArnoldShaderExport(const UsdStageRefPtr& _stage, UsdTimeCode _time_code) :
    m_stage(_stage), m_shaders_scope("/shaders"), m_time_code(_time_code) {
    UsdGeomScope::Define(m_stage, m_shaders_scope);
    ai.mtoa_init_export_session();
}

ArnoldShaderExport::~ArnoldShaderExport() {
    ai.mtoa_destroy_export_session();
}

bool
ArnoldShaderExport::is_valid() {
    return ai.is_valid();
}

void
ArnoldShaderExport::export_parameter(
    const AtNode* arnold_node, UsdAiShader& shader, const char* pname, uint8_t ptype, bool user) {
    if (ptype == AI_TYPE_ARRAY) {
        const auto arr = ai.NodeGetArray(arnold_node, pname);
        if (arr == nullptr || arr->nelements == 0 || arr->nkeys == 0 || arr->type == AI_TYPE_ARRAY) { return; }
        const auto itype = get_array_type(arr->type);
        if (itype == nullptr) { return; }
        auto param = shader.CreateParameter(TfToken(pname), itype->type);
        if (itype->f != nullptr) {
            itype->f(param, arr);
        }
    } else {
        const auto itype = get_simple_type(ptype);
        if (itype == nullptr) { return; }
        if (user) {
            UsdAiNodeAPI api(shader.GetPrim());
            auto param = api.CreateUserAttribute(TfToken(pname), itype->type);
            param.Set(itype->f(arnold_node, pname));
        } else {
            auto param = shader.CreateParameter(TfToken(pname), itype->type);
            if (!param) { return; }
            // These checks can't be easily moved out, or we'll just put complexity somewhere else.
            // Accessing AtParam directly won't give us the "type" checks the arnold functions are doing.
            if (itype->f != nullptr) {
                param.Set(itype->f(arnold_node, pname));
            }
            int32_t comp = -1;
            const auto linked_node = ptype == AI_TYPE_NODE ?
                                     reinterpret_cast<AtNode*>(ai.NodeGetPtr(arnold_node, pname)) :
                                     ai.NodeGetLink(arnold_node, pname, &comp);
            if (linked_node != nullptr) {
                const auto linked_path = write_arnold_node(linked_node, m_shaders_scope);
                if (linked_path.IsEmpty()) { return; }
                auto linked_prim = m_stage->GetPrimAtPath(linked_path);
                if (!linked_prim.IsValid()) { return; }
                UsdShadeShader linked_shader(linked_prim);
                const auto linked_output_type = ptype == AI_TYPE_NODE ?
                                                AI_TYPE_NODE :
                                                ai.NodeEntryGetOutputType(ai.NodeGetNodeEntry(linked_node));
                const auto& out_comp = out_comp_name(linked_output_type, comp);
                auto out_param = linked_shader.GetOutput(out_comp.n);
                if (out_param) {
                    param.ConnectToSource(out_param);
                } else {
                    // The automatic conversion will be handled by arnold, so make the full connection
                    // something neutral, like Token. Otherwise it's always float, there are no
                    // multicomponent, no float types in arnold.
                    param.ConnectToSource(
                        linked_shader.CreateOutput(out_comp.n, out_comp.t));
                }
            }
        }
    }
}

SdfPath
ArnoldShaderExport::write_arnold_node(const AtNode* arnold_node, SdfPath parent_path) {
    if (arnold_node == nullptr) { return SdfPath(); }
    const auto nentry = ai.NodeGetNodeEntry(arnold_node);
    if (ai.NodeEntryGetType(nentry) != AI_NODE_SHADER) {
        return SdfPath();
    } else {
        const auto it = m_shader_to_usd_path.find(arnold_node);
        if (it != m_shader_to_usd_path.end()) {
            return it->second;
        } else {
            std::string arnold_name_cleanup(ai.NodeGetName(arnold_node));
            // MtoA exports sub shaders with @ prefix, which is used for something else in USD
            // TODO: implement a proper cleanup using boost::regex
            std::replace(arnold_name_cleanup.begin(), arnold_name_cleanup.end(), '@', '_');
            auto shader_path = parent_path.AppendPath(SdfPath(arnold_name_cleanup));
            auto shader = UsdAiShader::Define(m_stage, shader_path);
            m_shader_to_usd_path.insert(std::make_pair(arnold_node, shader_path));

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
            return shader_path;
        }
    }
}

SdfPath
ArnoldShaderExport::export_shader(MObject obj) {
    if (!obj.hasFn(MFn::kShadingEngine)) { return SdfPath(); }
    auto arnold_node = ai.mtoa_export_node(&obj, "message");
    if (arnold_node == nullptr) { return SdfPath(); }
    // we can't store the material in the map
    MFnDependencyNode node(obj);
    auto material_path = m_shaders_scope.AppendChild(TfToken(node.name().asChar()));
    auto material_prim = m_stage->GetPrimAtPath(material_path);
    if (material_prim.IsValid()) { return material_path; } // if it already exists and setup
    auto material = UsdShadeMaterial::Define(m_stage, material_path);
    material_prim = material.GetPrim();
    auto shading_engine_path = write_arnold_node(arnold_node, material_path);
    if (!shading_engine_path.IsEmpty()) {
        auto rel = material_prim.CreateRelationship(TfToken("ai:surface"));
        rel.AppendTarget(shading_engine_path);
    }
    return material_path;
}

void
ArnoldShaderExport::setup_shaders(const MDagPath& dg, const SdfPath& path) {
    const auto obj = dg.node();
    if (obj.hasFn(MFn::kTransform) || obj.hasFn(MFn::kLocator)) { return; }
    MFnDependencyNode node(obj);
    auto plug = node.findPlug("instObjGroups");
    MPlugArray conns;
    plug.elementByLogicalIndex(dg.instanceNumber()).connectedTo(conns, false, true);
    const auto conns_length = conns.length();
    for (auto i = 0u; i < conns_length; ++i) {
        const auto splug = conns[i];
        auto sobj = splug.node();
        if (sobj.apiType() == MFn::kShadingEngine) {
            auto shader_path = export_shader(sobj);
            if (shader_path.IsEmpty()) { return; }
            auto shader = m_stage->GetPrimAtPath(shader_path);
            if (!shader.IsValid()) { return; }
            auto shape_prim = m_stage->GetPrimAtPath(path);
            if (!shape_prim.IsValid()) { return; }
            if (shape_prim.HasRelationship(UsdShadeTokens->materialBinding)) {
                auto rel = shape_prim.GetRelationship(UsdShadeTokens->materialBinding);
                rel.ClearTargets(true);
                rel.AppendTarget(shader_path);
            } else {
                auto rel = shape_prim.CreateRelationship(UsdShadeTokens->materialBinding);
                rel.AppendTarget(shader_path);
            }
            return;
        }
    }
}
