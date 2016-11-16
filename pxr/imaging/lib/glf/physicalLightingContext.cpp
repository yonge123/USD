#include "pxr/imaging/lib/glf/physicalLightingContext.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"

#include <map>
#include <array>

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((lightingUB, "PhysicalLighting"))
);

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

void GlfPhysicalLightingContext::WriteDefinitions(std::ostream& os) const
{
    // performance is not critical here, so we are going for an approach
    // that's simpler to define
    const size_t numLights = _useLighting ? _lights.size() : 0;
    os << "#define NUM_PHYSICAL_LIGHTS " << numLights << std::endl;
    if (numLights != 0)
    {
        std::map<PhysicalLightTypes, std::pair<size_t, std::string>> definitions = {
            {PHYSICAL_LIGHT_DISTANT, {0, "NUM_DISTANT_LIGHTS"}},
            {PHYSICAL_LIGHT_SPHERE, {0, "NUM_DISTANT_SPHERE"}},
            {PHYSICAL_LIGHT_SPOT, {0, "NUM_DISTANT_SPOT"}},
            {PHYSICAL_LIGHT_QUAD, {0, "NUM_DISTANT_QUAD"}},
            {PHYSICAL_LIGHT_SKY, {0, "NUM_DISTANT_SKY"}},
        };

        for (const auto& light : _lights)
            ++(definitions[light.GetLightType()].first);

        for (const auto& definition : definitions)
            os << "#define " << definition.second.second << " " << definition.second.first << std::endl;
    }
}

GlfPhysicalLightingContext::GlfPhysicalLightingContext() :
    _useLighting(false),
    _lightingUniformBlockValid(false)
{

}

GlfPhysicalLightingContext::~GlfPhysicalLightingContext()
{

}
