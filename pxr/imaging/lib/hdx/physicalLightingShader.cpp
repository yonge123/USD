#include "pxr/imaging/lib/hdx/physicalLightingShader.h"

#include "pxr/imaging/hdx/package.h"

HdxPhysicalLightingShader::HdxPhysicalLightingShader() :
    _lightingContext(GlfPhysicalLightingContext::New()),
    _bindingMap(TfCreateRefPtr(new GlfBindingMap())),
    _useLighting(true)
{
    _lightingContext->InitUniformBlockBindings(_bindingMap);
    _glslfx.reset(new GlfGLSLFX(HdxPackagePhysicalLightingShader()));
}

HdxPhysicalLightingShader::~HdxPhysicalLightingShader()
{

}

HdxPhysicalLightingShader::ID HdxPhysicalLightingShader::ComputeHash() const
{
    TfToken glslfxFile = HdxPackageSimpleLightingShader();
    size_t numLights = _useLighting ? static_cast<size_t>(_lightingContext->GetNumLightsUsed()) : 0;

    size_t hash = glslfxFile.Hash();
    boost::hash_combine(hash, numLights);

    return (ID)hash;
}

std::string HdxPhysicalLightingShader::GetSource(const TfToken& shaderStageKey) const
{
    std::stringstream ss;
    _lightingContext->WriteDefinitions(ss);
    ss << _glslfx->GetSource(shaderStageKey);
    return ss.str();
}

void HdxPhysicalLightingShader::BindResources(const Hd_ResourceBinder& binder, int program)
{
    _bindingMap->AssignUniformBindingsToProgram(program);
    _lightingContext->BindUniformBlocks(_bindingMap);
}

void HdxPhysicalLightingShader::SetCamera(const GfMatrix4d& worldToViewMatrix,
                       const GfMatrix4d& projectionMatrix)
{
    _lightingContext->SetCamera(worldToViewMatrix, projectionMatrix);
}
