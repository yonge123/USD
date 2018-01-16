//
// Created by palm on 1/4/17.
//

#ifndef _usdExport_MayaImagePlaneWriter_h_
#define _usdExport_MayaImagePlaneWriter_h_

#include "pxr/pxr.h"
#include "usdMaya/MayaPrimWriter.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomImagePlane;

class MayaImagePlaneWriter : public MayaPrimWriter {
public:
    MayaImagePlaneWriter(const MDagPath & iDag, const SdfPath& uPath, usdWriteJobCtx& job);
    virtual ~MayaImagePlaneWriter();

    virtual void postExport() override;    
    virtual void write(const UsdTimeCode& usdTime) override;
    virtual bool isShapeAnimated() const override;

    enum {
        IMAGE_PLANE_FIT_FILL,
        IMAGE_PLANE_FIT_BEST,
        IMAGE_PLANE_FIT_HORIZONTAL,
        IMAGE_PLANE_FIT_VERTICAL,
        IMAGE_PLANE_FIT_TO_SIZE
    };

    static const TfToken image_plane_fill;
    static const TfToken image_plane_best;
    static const TfToken image_plane_horizontal;
    static const TfToken image_plane_vertical;
    static const TfToken image_plane_to_size;
protected:
    bool writeImagePlaneAttrs(const UsdTimeCode& usdTime, UsdGeomImagePlane& primSchema);

    bool mIsShapeAnimated;
};

using MayaImagePlaneWriterPtr = std::shared_ptr<MayaImagePlaneWriter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif //_usdExport_MayaImagePlaneWriter_h_
