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
    MayaImagePlaneWriter(const MDagPath & iDag, const SdfPath& uPath, bool instanceSource, usdWriteJobCtx& jobCtx);
    virtual ~MayaImagePlaneWriter();
    virtual void write(const UsdTimeCode& usdTime) override;
    virtual bool isShapeAnimated() const override;
protected:
    bool writeImagePlaneAttrs(const UsdTimeCode& usdTime, UsdGeomImagePlane& primSchema);

    bool mIsShapeAnimated;
};

using MayaImagePlaneWriterPtr = std::shared_ptr<MayaImagePlaneWriter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif //_usdExport_MayaImagePlaneWriter_h_
