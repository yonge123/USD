#pragma once

#include "pxr/usdImaging/usdImagingGL/engine.h"

class UsdImagingGLHkEngine : public UsdImagingGLEngine {
public:
    virtual void Render(const UsdPrim& root, RenderParams params) {
        std::cerr << "Calling render on hk engine" << std::endl;
    };

    virtual void InvalidateBuffers() {
        std::cerr << "Calling invalidate buffers on hk engine" << std::endl;
    };
};
