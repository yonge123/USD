#include "pxr/usdImaging/usdImagingGL/imagePlaneAdapter.h"

#include "pxr/usdImaging/usdImagingGL/textureUtils.h"
#include "pxr/usd/usdGeom/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingGLImagePlaneAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingGLImagePlaneAdapter::~UsdImagingGLImagePlaneAdapter() {

}

HdTextureResource::ID
UsdImagingGLImagePlaneAdapter::GetTextureResourceID(
    const UsdPrim& usdPrim, const SdfPath& id,
    UsdTimeCode time, size_t salt) const {
    return UsdImagingGL_GetTextureResourceID(usdPrim, id.AppendProperty(UsdGeomTokens->infoFilename), time, salt);
}

HdTextureResourceSharedPtr
UsdImagingGLImagePlaneAdapter::GetTextureResource(
    const UsdPrim& usdPrim,
    const SdfPath& id,
    UsdTimeCode time) const {
    return UsdImagingGL_GetTextureResource(usdPrim, id.AppendProperty(UsdGeomTokens->infoFilename), time);
}

PXR_NAMESPACE_CLOSE_SCOPE
