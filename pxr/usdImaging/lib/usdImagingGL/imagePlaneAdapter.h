#ifndef USDIMAGINGGL_IMAGEPLANE_ADAPTER_H
#define USDIMAGINGGL_IMAGEPLANE_ADAPTER_H

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdImaging/imagePlaneAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingGLImagePlaneAdapter : public UsdImagingImagePlaneAdapter {
public:
    using BaseAdapter = UsdImagingImagePlaneAdapter;

    UsdImagingGLImagePlaneAdapter() : UsdImagingImagePlaneAdapter() { }

    USDIMAGING_API
    virtual ~UsdImagingGLImagePlaneAdapter();

    USDIMAGING_API
    virtual HdTextureResource::ID
    GetTextureResourceID(const UsdPrim& usdPrim, const SdfPath& id,
                         UsdTimeCode time, size_t salt) const override;

    USDIMAGING_API
    virtual HdTextureResourceSharedPtr
    GetTextureResource(const UsdPrim& usdPrim, const SdfPath& id,
                       UsdTimeCode time) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGINGGL_IMAGEPLANE_ADAPTER_H
