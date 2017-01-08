#pragma once

#include "pxr/usdImaging/usdImagingGL/engine.h"
#include <memory>

class UsdImagingGLHkEngine : public UsdImagingGLEngine {
public:
    UsdImagingGLHkEngine(const SdfPathVector& excluded_paths);
    ~UsdImagingGLHkEngine();

    UsdImagingGLHkEngine(const UsdImagingGLHkEngine& other) = delete;
    virtual void Render(const UsdPrim& root, RenderParams params);
    virtual void SetCameraState(const GfMatrix4d& viewMatrix,
                                const GfMatrix4d& projectionMatrix,
                                const GfVec4d& viewport);
    virtual void SetLightingState(GlfSimpleLightVector const &lights,
                                  GlfSimpleMaterial const &material,
                                  GfVec4f const &sceneAmbient);
    virtual void InvalidateBuffers();
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};
