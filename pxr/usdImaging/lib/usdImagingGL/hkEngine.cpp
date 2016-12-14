#include "hkEngine.h"

namespace {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}

struct UsdImagingGLHkEngine::Impl {
    Impl() : something(1)
    { }

    ~Impl()
    { }

    int something;
};

UsdImagingGLHkEngine::UsdImagingGLHkEngine() : p_impl(make_unique<UsdImagingGLHkEngine::Impl>())
{

}

UsdImagingGLHkEngine::~UsdImagingGLHkEngine()
{

}
