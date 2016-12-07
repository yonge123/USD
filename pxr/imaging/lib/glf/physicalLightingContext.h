#pragma once

#include "physicalLight.h"
#include "uniformBlock.h"
#include "bindingMap.h"
#include "lightingContext.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4f.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"

#include <ostream>
#include <array>
#include <memory>

typedef std::shared_ptr<class GlfPhysicalLightingContext> GlfPhysicalLightingContextRefPtr;
typedef std::weak_ptr<class GlfPhysicalLightingContext> GlfPhysicalLightingContextPtr;

class GlfPhysicalLightingContext : public GlfLightingContext {
public:
    typedef GlfPhysicalLightingContext This;
    static GlfPhysicalLightingContextRefPtr New();

    void SetLights(const GlfPhysicalLightVector& lights);
    GlfPhysicalLightVector& GetLights();

    void SetCamera(const GfMatrix4d& worldToViewMatrix,
                   const GfMatrix4d& projectionMatrix);

    int GetNumLightsUsed() const;

    void SetUseLighting(bool val);
    bool GetUseLighting() const;

    void WriteDefinitions(std::ostream& os) const;

    bool GetUseShadows() const { return false; };

    void InitUniformBlockBindings(GlfBindingMapPtr const &bindingMap) const;
    void InitSamplerUnitBindings(GlfBindingMapPtr const &bindingMap) const { }

    void BindUniformBlocks(GlfBindingMapPtr const &bindingMap);
    void BindSamplers(GlfBindingMapPtr const &bindingMap) { }

    void UnbindSamplers(GlfBindingMapPtr const &bindingMap) { }

    void SetStateFromOpenGL() { }

protected:
    GlfPhysicalLightingContext();
    ~GlfPhysicalLightingContext();
private:
    typedef std::array<size_t, PHYSICAL_LIGHT_MAX> LightCountArray;
    void CountLights(LightCountArray& lightCount) const;
    GfMatrix4d _worldToViewMatrix;
    GfMatrix4d _projectionMatrix;

    GlfPhysicalLightVector _lights;

    GlfUniformBlockRefPtr _lightingUniformBlock;

    bool _useLighting;
    bool _lightingUniformBlockValid;
};
