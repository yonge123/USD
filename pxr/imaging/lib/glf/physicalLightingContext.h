#pragma once

#include "physicalLight.h"
#include "uniformBlock.h"
#include "bindingMap.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4f.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"

#include <ostream>
#include <array>

TF_DECLARE_WEAK_AND_REF_PTRS(GlfPhysicalLightingContext);

class GlfPhysicalLightingContext : public TfRefBase, public TfWeakBase {
public:
    typedef GlfPhysicalLightingContext This;
    static GlfPhysicalLightingContextRefPtr New();

    void SetLights(const GlfPhysicalLightVector& lights);
    GlfPhysicalLightVector& GetLights();

    void SetCamera(const GfMatrix4d& worldToViewMatrix,
                   const GfMatrix4d& projectionMatrix);

    int GetNumLightsUsed() const;

    void SetUseLighting(bool val);
    bool GetUseLigthing() const;

    void WriteDefinitions(std::ostream& os) const;

    void InitUniformBlockBindings(const GlfBindingMapPtr&bindingMap) const;
    void BindUniformBlocks(const GlfBindingMapPtr &bindingMap);
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
