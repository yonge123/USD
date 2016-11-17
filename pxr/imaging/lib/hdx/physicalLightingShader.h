#pragma once

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/lightingShader.h"
#include "pxr/imaging/hd/resource.h"

#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/physicalLightingContext.h"

#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <string>

TF_DECLARE_WEAK_AND_REF_PTRS(GlfBindingMap);

/// \class HdxPhysicalLightingShader
///
/// A shader that supports physical lighting functionality.
///
class HdxPhysicalLightingShader : public HdLightingShader {
public:
    HdxPhysicalLightingShader();
    virtual ~HdxPhysicalLightingShader();

    virtual void SetCamera(const GfMatrix4d& worldToViewMatrix,
                           const GfMatrix4d& projectionMatrix);

    /// HdShader overrides
    virtual ID ComputeHash() const;
    virtual std::string GetSource(TfToken const &shaderStageKey) const;
    virtual void BindResources(const Hd_ResourceBinder& binder, int program);
    /*virtual void UnbindResources(Hd_ResourceBinder const &binder, int program);
    virtual void AddBindings(HdBindingRequestVector *customBindings);

    /// HdLightingShader overrides
    virtual void SetCamera(GfMatrix4d const &worldToViewMatrix,
                           GfMatrix4d const &projectionMatrix);

    void SetLightingStateFromOpenGL();
    void SetLightingState(GlfSimpleLightingContextPtr const &lightingContext);*/

    GlfPhysicalLightingContextRefPtr GetLightingContext() {
        return _lightingContext;
    };

private:
    GlfPhysicalLightingContextRefPtr _lightingContext;
    GlfBindingMapRefPtr _bindingMap;
    bool _useLighting;
    boost::scoped_ptr<GlfGLSLFX> _glslfx;
};

typedef boost::shared_ptr<class HdxPhysicalLightingShader> HdxPhysicalLightingShaderSharedPtr;
