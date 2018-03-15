#ifndef HD_IMAGEPLANE_H
#define HD_IMAGEPLANE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/rprim.h"

#include "pxr/imaging/hd/enums.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdImagePlaneReprDesc {
    HdImagePlaneReprDesc(HdImagePlaneGeomStyle _geomStyle = HdImagePlaneGeomStyleInvalid)
        : geomStyle(_geomStyle) { }

    HdImagePlaneGeomStyle geomStyle;
};

class HdImagePlane : public HdRprim {
public:
    HD_API
    virtual ~HdImagePlane();

    HD_API
    static void ConfigureRepr(
        const TfToken& reprName,
        const HdImagePlaneReprDesc& desc);
protected:
    HD_API
    HdImagePlane(
        const SdfPath& id,
        const SdfPath& instancerId = SdfPath());

    using _ImagePlaneReprConfig = _ReprDescConfigs<HdImagePlaneReprDesc>;

    HD_API
    static _ImagePlaneReprConfig::DescArray _GetReprDesc(const TfToken& reprName);
private:
    static _ImagePlaneReprConfig _reprDescConfig;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
