#ifndef USDMAYA_ARNOLD_SHADER_EXPORT_H
#define USDMAYA_ARNOLD_SHADER_EXPORT_H

#include "pxr/usd/usdAi/aiShader.h"

#include <maya/MObject.h>
#include <maya/MDagPath.h>

struct AtNode;

class ArnoldShaderExport {
public:
    ArnoldShaderExport(const UsdStageRefPtr& _stage, UsdTimeCode _time_code);
    ~ArnoldShaderExport();

    static bool is_valid();
private:
    std::map<const AtNode*, SdfPath> m_shader_to_usd_path;
    const UsdStageRefPtr& m_stage;
    SdfPath m_shaders_scope;
    UsdTimeCode m_time_code;

    void export_parameter(const AtNode* arnold_node, UsdAiShader& shader, const char* pname, uint8_t ptype, bool user);
    SdfPath write_arnold_node(const AtNode* arnold_node, SdfPath parent_path);
public:
    SdfPath export_shader(MObject obj);
    void setup_shaders(const MDagPath& dg, const SdfPath& path);
};

#endif
