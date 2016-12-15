#pragma once

#include "pxr/usdImaging/usdImagingGL/engine.h"
#include <memory>

class UsdImagingGLHkEngine : public UsdImagingGLEngine {
public:
    UsdImagingGLHkEngine();
    ~UsdImagingGLHkEngine();

    UsdImagingGLHkEngine(const UsdImagingGLHkEngine& other) = delete;
    virtual void Render(const UsdPrim& root, RenderParams params);
    virtual void InvalidateBuffers();
private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};
