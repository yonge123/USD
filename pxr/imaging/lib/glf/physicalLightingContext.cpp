#include "pxr/imaging/lib/glf/physicalLightingContext.h"

GlfPhysicalLightingContextRefPtr GlfPhysicalLightingContext::New()
{
    return TfCreateRefPtr(new This());
}

void GlfPhysicalLightingContext::SetLights(const GlfPhysicalLightVector& lights)
{
    _lights = lights;
    _lightingUniformBlockValid = false;
}

void GlfPhysicalLightingContext::SetCamera(const GfMatrix4d& worldToViewMatrix,
                                           const GfMatrix4d& projectionMatrix)
{
    if (_worldToViewMatrix != worldToViewMatrix || _projectionMatrix != projectionMatrix)
    {
        _worldToViewMatrix = worldToViewMatrix;
        _projectionMatrix = projectionMatrix;
        _lightingUniformBlockValid = false;
    }
}

GlfPhysicalLightVector& GlfPhysicalLightingContext::GetLights()
{
    return _lights;
}

int GlfPhysicalLightingContext::GetNumLightsUsed() const
{
    return static_cast<int>(_lights.size());
}

void GlfPhysicalLightingContext::SetUseLighting(bool val)
{
    if (_useLighting != val)
    {
        _useLighting = val;
        _lightingUniformBlockValid = false;
    }
}

bool GlfPhysicalLightingContext::GetUseLigthing() const
{
    return _useLighting;
}

GlfPhysicalLightingContext::GlfPhysicalLightingContext() :
    _useLighting(false),
    _lightingUniformBlockValid(false)
{

}

GlfPhysicalLightingContext::~GlfPhysicalLightingContext()
{

}
