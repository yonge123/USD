#ifndef USDMAYA_ARNOLD_SHADER_EXPORT_H
#define USDMAYA_ARNOLD_SHADER_EXPORT_H

#include "pxr/usd/usdAi/aiShader.h"

#include <maya/MObject.h>

struct AtNode;
struct AtArray;

class ArnoldShaderExport {
public:
    ArnoldShaderExport(const UsdStageRefPtr& _stage, UsdTimeCode _time_code);
    ~ArnoldShaderExport();

    static bool is_valid();

    // TODO: use sorted vectors and arrays with lower bound lookups
    struct simple_type {
        const SdfValueTypeName& type;
        std::function<VtValue(const AtNode*, const char*)> f;

        simple_type(const SdfValueTypeName& _type, std::function<VtValue(const AtNode*, const char*)> _f) :
            type(_type), f(_f) { }
    };

    struct array_type {
        const SdfValueTypeName& type;
        std::function<void(UsdShadeParameter&, const AtArray*)> f;

        array_type(const SdfValueTypeName& _type, std::function<void(UsdShadeParameter&, const AtArray*)> _f) :
            type(_type), f(_f) { }
    };
private:
    simple_type m_node_simple_type;
    array_type m_node_array_type;
    std::map<const AtNode*, UsdPrim> m_shader_to_usd_path;
    const UsdStageRefPtr& m_stage;
    SdfPath m_shaders_scope;
    UsdTimeCode m_time_code;

    const simple_type* get_simple_type(uint8_t type);
    const array_type* get_array_type(uint8_t type);
    void export_parameter(const AtNode* arnold_node, UsdAiShader& shader, const char* pname, uint8_t ptype, bool user);
    UsdPrim write_arnold_node(const AtNode* arnold_node);
public:
    UsdPrim export_shader(MObject obj, const char* attr);
};

#endif
