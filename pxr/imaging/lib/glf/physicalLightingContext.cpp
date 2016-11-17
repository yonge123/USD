#include "pxr/imaging/lib/glf/physicalLightingContext.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"

#include <map>
#include <array>

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((physicalLightingUB, "PhysicalLighting"))
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
    const size_t numLights = _lights.size();
    os << "#define NUM_PHYSICAL_LIGHTS " << numLights << std::endl;
    if (numLights != 0)
    {
        const static std::map<PhysicalLightTypes, const char*> definitions = {
            {PHYSICAL_LIGHT_DISTANT, "NUM_DISTANT_LIGHTS"},
            {PHYSICAL_LIGHT_SPHERE, "NUM_SPHERE_LIGHTS"},
            {PHYSICAL_LIGHT_SPOT, "NUM_SPOT_LIGHTS"},
            {PHYSICAL_LIGHT_QUAD, "NUM_QUAD_LIGHTS"},
            {PHYSICAL_LIGHT_SKY, "NUM_SKY_LIGHTS"}
        };

        LightCountArray lightCount = {0};
        CountLights(lightCount);

        for (const auto& definition : definitions)
            os << "#define " << definition.second << " " << lightCount[definition.first] << std::endl;
    }
}

void GlfPhysicalLightingContext::InitUniformBlockBindings(const GlfBindingMapPtr&bindingMap) const
{
    bindingMap->GetUniformBinding(_tokens->physicalLightingUB);
}

// no templated lambdas in c++11 :(
template<typename T, PhysicalLightTypes lightType>
T* GetLightFromSlab(int8_t* slab, std::array<size_t, PHYSICAL_LIGHT_MAX + 1>& offsets)
{
    T* ret = reinterpret_cast<T*>(slab[offsets[lightType]]);
    offsets[lightType] += sizeof(T);
    return ret;
}

// Our goal is to align the data to 16 bytes, that plays better with the gpu data layout
// And use ints instread of bools.
// We are creating one block for each type of light, so iterating through them will generate
// less divergence and we can work with less registers when handling each light type
void GlfPhysicalLightingContext::BindUniformBlocks(const GlfBindingMapPtr &bindingMap)
{
    if (not _lightingUniformBlock)
        _lightingUniformBlock = GlfUniformBlock::New();

    if (not _lightingUniformBlockValid)
    {
        LightCountArray lightCount = {0};
        CountLights(lightCount);

        struct PhysicalLight {
            float position[4];
            float color[4];
            float intensity; float specular; float diffuse; float indirect;
            float attenuation[2]; int hasShadows; int padding;
        };

        struct DistantLight {
            PhysicalLight base;
            float spread; int padding[3];
        };

        struct SphereLight {
            PhysicalLight base;
            float radius; int padding[3];
        };

        struct SpotLight {
            PhysicalLight base;
            float direction[4];
            float radius; float coneAngle; float penumbraAngle; int padding;
        };

        struct QuadLight {
            PhysicalLight base;
            float vertex0[4];
            float vertex1[4];
            float vertex2[4];
            float vertex3[4];
        };

        struct SkyLight {
            PhysicalLight base;
        };

        struct PhysicalLighting {
            int32_t useLighting;
            int32_t padding[3];
            int8_t lightData[0];
        };

        const static std::map<int, size_t> lightStructSizes = {
            {PHYSICAL_LIGHT_DISTANT, sizeof(DistantLight)},
            {PHYSICAL_LIGHT_SPHERE, sizeof(SphereLight)},
            {PHYSICAL_LIGHT_SPOT, sizeof(SpotLight)},
            {PHYSICAL_LIGHT_QUAD, sizeof(QuadLight)},
            {PHYSICAL_LIGHT_SKY, sizeof(SkyLight)}
        };

        std::array<size_t, PHYSICAL_LIGHT_MAX + 1> lightStructOffsets = {0};
        for (int lightType = 0; lightType < PHYSICAL_LIGHT_MAX; ++lightType)
            lightStructOffsets[lightType + 1] = lightStructOffsets[lightType] +
                lightStructSizes.find(lightType)->second * lightCount[lightType];

        const size_t totalDataSize = lightStructOffsets[PHYSICAL_LIGHT_MAX] + sizeof(int32_t) * 4;
        std::vector<int8_t> memorySlab(totalDataSize, 0);
        PhysicalLighting* lightUniformData = reinterpret_cast<PhysicalLighting*>(memorySlab.data());

        lightUniformData->useLighting = _useLighting;

        // TODO: transform to camera space!
        auto fillBaseLightData = [] (PhysicalLight& base, const GlfPhysicalLight& light, bool hasPosition) {
            if (hasPosition)
            {
                const auto position = light.GetTransform().Transform(GfVec3f(0.0f, 0.0f, 0.0f));
                base.position[0] = position[0];
                base.position[1] = position[1];
                base.position[2] = position[2];
            }
            const auto& color = light.GetColor();
            base.color[0] = color[0];
            base.color[1] = color[1];
            base.color[2] = color[2];
            base.intensity = light.GetIntensity();
            base.diffuse = light.GetDiffuse();
            base.specular = light.GetSpecular();
            base.indirect = light.GetIndirect();
            const auto& attenuation = light.GetAttenuation();
            base.attenuation[0] = attenuation[0];
            base.attenuation[1] = attenuation[1];
            base.hasShadows = light.GetHasShadow() ? 1 : 0;
        };

        for (const auto& light : _lights)
        {
            const auto lightType = light.GetLightType();

            switch (lightType)
            {
                case PHYSICAL_LIGHT_DISTANT:
                {
                    auto lightData = GetLightFromSlab<DistantLight, PHYSICAL_LIGHT_DISTANT>(lightUniformData->lightData, lightStructOffsets);
                    fillBaseLightData(lightData->base, light, false);
                    const auto direction = light.GetTransform().TransformDir(light.GetDirection());
                    lightData->base.position[0] = direction[0];
                    lightData->base.position[1] = direction[1];
                    lightData->base.position[2] = direction[2];
                    lightData->spread = light.GetSpread();
                }
                    break;
                case PHYSICAL_LIGHT_SPHERE:
                {
                    auto lightData = GetLightFromSlab<SphereLight, PHYSICAL_LIGHT_SPHERE>(lightUniformData->lightData, lightStructOffsets);
                    fillBaseLightData(lightData->base, light, true);
                    lightData->radius = light.GetRadius();
                }
                    break;
                case PHYSICAL_LIGHT_SPOT:
                {
                    auto lightData = GetLightFromSlab<SpotLight, PHYSICAL_LIGHT_SPOT>(lightUniformData->lightData, lightStructOffsets);
                    fillBaseLightData(lightData->base, light, true);
                    const auto direction = light.GetTransform().TransformDir(light.GetDirection());
                    lightData->direction[0] = direction[0];
                    lightData->direction[1] = direction[1];
                    lightData->direction[2] = direction[2];
                    lightData->coneAngle = light.GetConeAngle();
                    lightData->penumbraAngle = light.GetPenumbraAngle();
                }
                    break;
                case PHYSICAL_LIGHT_QUAD:
                {
                    auto lightData = GetLightFromSlab<QuadLight, PHYSICAL_LIGHT_QUAD>(lightUniformData->lightData, lightStructOffsets);
                    fillBaseLightData(lightData->base, light, true);
                    const auto& vertex0 = light.GetTransform().Transform(GfVec3f(-0.5f, -0.5f, 0.0));
                    const auto& vertex1 = light.GetTransform().Transform(GfVec3f(0.5f, -0.5f, 0.0));
                    const auto& vertex2 = light.GetTransform().Transform(GfVec3f(0.5f, 0.5f, 0.0));
                    const auto& vertex3 = light.GetTransform().Transform(GfVec3f(-0.5f, 0.5f, 0.0));
                    lightData->vertex0[0] = vertex0[0];
                    lightData->vertex0[1] = vertex0[1];
                    lightData->vertex0[2] = vertex0[2];
                    lightData->vertex1[0] = vertex1[0];
                    lightData->vertex1[1] = vertex1[1];
                    lightData->vertex1[2] = vertex1[2];
                    lightData->vertex2[0] = vertex2[0];
                    lightData->vertex2[1] = vertex2[1];
                    lightData->vertex2[2] = vertex2[2];
                    lightData->vertex3[0] = vertex3[0];
                    lightData->vertex3[1] = vertex3[1];
                    lightData->vertex3[2] = vertex3[2];
                }
                    break;
                case PHYSICAL_LIGHT_SKY:
                {
                    auto lightData = GetLightFromSlab<SkyLight, PHYSICAL_LIGHT_SKY>(lightUniformData->lightData, lightStructOffsets);
                    fillBaseLightData(lightData->base, light, true);
                }
                    break;
                default:
                    break;
            }
        }

        _lightingUniformBlock->Update(lightUniformData, totalDataSize);
        _lightingUniformBlockValid = true;
    }

    _lightingUniformBlock->Bind(bindingMap, _tokens->physicalLightingUB);
}

void GlfPhysicalLightingContext::CountLights(LightCountArray& lightCount) const
{
    for (const auto& light : _lights)
        ++lightCount[light.GetLightType()];
}

GlfPhysicalLightingContext::GlfPhysicalLightingContext() :
    _useLighting(false),
    _lightingUniformBlockValid(false)
{

}

GlfPhysicalLightingContext::~GlfPhysicalLightingContext()
{

}
