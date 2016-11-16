#pragma once

#include "pxr/imaging/lib/glf/physicalLight.h"
#include "pxr/imaging/lib/glf/uniformBlock.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4f.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"

#include <ostream>

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
protected:
    GlfPhysicalLightingContext();
    ~GlfPhysicalLightingContext();
private:
    GfMatrix4d _worldToViewMatrix;
    GfMatrix4d _projectionMatrix;

    GlfPhysicalLightVector _lights;

    GlfUniformBlockRefPtr _lightingUniformBlock;

    bool _useLighting;
    bool _lightingUniformBlockValid;
};
