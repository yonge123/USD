#include "pxr/usdImaging/usdImaging/imagePlaneAdapter.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"

#include "pxr/imaging/hd/imagePlane.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingImagePlaneAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingImagePlaneAdapter::~UsdImagingImagePlaneAdapter() {

}

SdfPath
UsdImagingImagePlaneAdapter::Populate(
    const UsdPrim& prim,
    UsdImagingIndexProxy* index,
    const UsdImagingInstancerContext* instancerContext /*= nullptr*/){
    return HdImagePlane::IsEnabled() ? _AddRprim(HdPrimTypeTokens->imagePlane,
                     prim, index, GetMaterialId(prim), instancerContext) : SdfPath();
}

void
UsdImagingImagePlaneAdapter::TrackVariability(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    HdDirtyBits* timeVaryingBits,
    const UsdImagingInstancerContext* instancerContext) const {
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);

    _IsVarying(
        prim,
        UsdGeomTokens->points,
        HdChangeTracker::DirtyPoints,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);

    if (!_IsVarying(
            prim,
            UsdGeomTokens->faceVertexCounts,
            HdChangeTracker::DirtyTopology,
            UsdImagingTokens->usdVaryingTopology,
            timeVaryingBits, false)) {
        _IsVarying(prim,
            UsdGeomTokens->faceVertexIndices,
            HdChangeTracker::DirtyTopology,
            UsdImagingTokens->usdVaryingTopology,
            timeVaryingBits, false);
    }
}

void
UsdImagingImagePlaneAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const*
    instancerContext /*= nullptr*/) const {
    BaseAdapter::UpdateForTime(prim, cachePath, time, requestedBits, instancerContext);

    UsdImagingValueCache* valueCache = _GetValueCache();

    if (requestedBits & HdChangeTracker::DirtyPoints) {
        static const VtVec3fArray vertices = {
            GfVec3f(-1.0f ,  1.0f , 0.0f),
            GfVec3f( 1.0f ,  1.0f , 0.0f),
            GfVec3f( 1.0f , -1.0f , 0.0f),
            GfVec3f(-1.0f , -1.0f , 0.0f),
        };

        VtValue& pointsValues = valueCache->GetPoints(cachePath);
        pointsValues = vertices;

        _MergePrimvar(
            &valueCache->GetPrimvars(cachePath),
            HdTokens->points,
            HdInterpolationVertex,
            HdPrimvarRoleTokens->point);
    }

    if (requestedBits & HdChangeTracker::DirtyTopology) {
        VtValue& topology = valueCache->GetTopology(cachePath);

        // Simplest right handed vertex counts and vertex indices.
        static const VtIntArray faceVertexCounts = {
            3, 3
        };

        static const VtIntArray faceVertexIndices = {
            0, 1, 2, 0, 2, 3
        };

        static const VtIntArray holeIndices;

        topology = HdMeshTopology(
            UsdGeomTokens->triangleSubdivisionRule,
            UsdGeomTokens->rightHanded,
            faceVertexCounts,
            faceVertexIndices,
            holeIndices, 0);
    }
}

bool
UsdImagingImagePlaneAdapter::IsSupported(const UsdImagingIndexProxy* index) const {
    return index->IsRprimTypeSupported(HdPrimTypeTokens->imagePlane);
}

PXR_NAMESPACE_CLOSE_SCOPE
