#include "pxr/usdImaging/usdImaging/imagePlaneAdapter.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"

#include "pxr/usd/usdGeom/imagePlane.h"

#include "pxr/imaging/hd/imagePlane.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Simplest right handed vertex counts and vertex indices.
const VtIntArray _faceVertexCounts = {
    3, 3
};

const VtIntArray _faceVertexIndices = {
    0, 1, 2, 0, 2, 3
};

const VtIntArray _holeIndices;


}

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
    return HdImagePlane::IsEnabled() ? _AddRprim(HdPrimTypeTokens->mesh,
                     prim, index, GetMaterialId(prim), instancerContext) : SdfPath();
    // return HdImagePlane::IsEnabled() ? _AddRprim(HdPrimTypeTokens->imagePlane,
    //                                              prim, index, GetMaterialId(prim), instancerContext) : SdfPath();
}

void
UsdImagingImagePlaneAdapter::TrackVariability(
    const UsdPrim& prim,
    const SdfPath& cachePath,
    HdDirtyBits* timeVaryingBits,
    const UsdImagingInstancerContext* instancerContext) const {
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);
    *timeVaryingBits |= HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyExtent;
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

    if (requestedBits & HdChangeTracker::DirtyTransform) {
        // Update the transform with the size authored for the sphere.
        GfMatrix4d& ctm = valueCache->GetTransform(cachePath);
        GfMatrix4d xf; xf.SetIdentity();
        ctm = xf * ctm;
    }

    if (requestedBits & HdChangeTracker::DirtyPoints) {
        UsdGeomImagePlane imagePlane(prim);
        VtVec3fArray vertices;
        VtVec2fArray uvs;
        imagePlane.CalculateGeometry(&vertices, &uvs, time);
        valueCache->GetPoints(cachePath) = vertices;

        _MergePrimvar(
            &valueCache->GetPrimvars(cachePath),
            HdTokens->points,
            HdInterpolationVertex,
            HdPrimvarRoleTokens->point);

        valueCache->GetPrimvar(cachePath, TfToken("st")) = uvs;

        _MergePrimvar(
            &valueCache->GetPrimvars(cachePath),
            TfToken("st"),
            HdInterpolationVertex);
    }

    if (requestedBits & HdChangeTracker::DirtyExtent) {
        // This does not change the extent representation in the viewport,
        // but affects the frustrum culling.
        // This also freaks out the min / max depth calculation.
        GfRange3d extent;
        for (const auto& vertex: valueCache->GetPoints(cachePath).Get<VtVec3fArray>()) {
            extent.ExtendBy(vertex);
        }
        valueCache->GetExtent(cachePath) = extent;
    }

    if (requestedBits & HdChangeTracker::DirtyTopology) {
        VtValue& topology = valueCache->GetTopology(cachePath);

        topology = HdMeshTopology(
            UsdGeomTokens->triangleSubdivisionRule,
            UsdGeomTokens->rightHanded,
            _faceVertexCounts,
            _faceVertexIndices,
            _holeIndices, 0);
    }
}

bool
UsdImagingImagePlaneAdapter::IsSupported(const UsdImagingIndexProxy* index) const {
    return index->IsRprimTypeSupported(HdPrimTypeTokens->mesh);
    // return index->IsRprimTypeSupported(HdPrimTypeTokens->imagePlane);
}

PXR_NAMESPACE_CLOSE_SCOPE
