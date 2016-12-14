#pragma once

#include "pxr/usdImaging/usdImagingGL/engine.h"
#include <memory>

class UsdImagingGLHkEngine : public UsdImagingGLEngine {
public:
    UsdImagingGLHkEngine();
    ~UsdImagingGLHkEngine();

    UsdImagingGLHkEngine(const UsdImagingGLHkEngine& other) = delete;
    virtual void Render(const UsdPrim& root, RenderParams params) {
        std::cerr << "Calling render on hk engine" << std::endl;
    };

    virtual void InvalidateBuffers() {
        std::cerr << "Calling invalidate buffers on hk engine" << std::endl;
    };
private:
    struct Impl;
    std::unique_ptr<Impl> p_impl;
};
