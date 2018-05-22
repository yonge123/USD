#include "pxr/pxr.h"
#include "pxr/imaging/hd/imagePlane.h"

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

HdImagePlane::HdImagePlane(const SdfPath& id,
                           const SdfPath& instancerId)
    : HdRprim(id, instancerId) {

}

HdImagePlane::~HdImagePlane() {

}

HdImagePlane::_ImagePlaneReprConfig HdImagePlane::_reprDescConfig;

void
HdImagePlane::ConfigureRepr(const TfToken& reprName,
                            const HdImagePlaneReprDesc& desc) {
    HD_TRACE_FUNCTION();

    _reprDescConfig.Append(reprName, _ImagePlaneReprConfig::DescArray{desc});
}

bool
HdImagePlane::IsEnabled() {
    return false;
}

HdImagePlane::_ImagePlaneReprConfig::DescArray
HdImagePlane::_GetReprDesc(const TfToken& reprName) {
    return _reprDescConfig.Find(reprName);
}

PXR_NAMESPACE_CLOSE_SCOPE
