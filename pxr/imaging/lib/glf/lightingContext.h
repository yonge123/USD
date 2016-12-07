#ifndef GLF_LIGHTING_CONTEXT_H
#define GLF_LIGHTING_CONTEXT_H

#include <memory>

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4f.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"

typedef std::shared_ptr<class GlfLightingContext> GlfLightingContextSharedPtr;

TF_DECLARE_WEAK_AND_REF_PTRS(GlfBindingMap);
TF_DECLARE_WEAK_AND_REF_PTRS(GlfUniformBlock);

class GlfLightingContext {
public:
    // returns the effective number of lights taken into account
    // in composable/compatible shader constraints
    virtual int GetNumLightsUsed() const = 0;

    virtual void SetCamera(GfMatrix4d const &worldToViewMatrix,
                           GfMatrix4d const &projectionMatrix) = 0;

    virtual void SetUseLighting(bool val) = 0;
    virtual bool GetUseLighting() const = 0;

    // returns true if any light has shadow enabled.
    virtual bool GetUseShadows() const = 0;

    virtual void InitUniformBlockBindings(GlfBindingMapPtr const &bindingMap) const = 0;
    virtual void InitSamplerUnitBindings(GlfBindingMapPtr const &bindingMap) const = 0;

    virtual void BindUniformBlocks(GlfBindingMapPtr const &bindingMap) = 0;
    virtual void BindSamplers(GlfBindingMapPtr const &bindingMap) = 0;

    virtual void UnbindSamplers(GlfBindingMapPtr const &bindingMap) = 0;

    virtual void SetStateFromOpenGL() = 0;
};

#endif
