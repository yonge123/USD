//
// Created by palm on 1/4/17.
//

#ifndef _usdExport_MayaImagePlaneWriter_h_
#define _usdExport_MayaImagePlaneWriter_h_

#include "pxr/pxr.h"
#include "usdMaya/transformWriter.h"
#include "pxr/usd/usd/prim.h"

// Generating extra shader definitions for real-time display.
#define GENERATE_SHADERS

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomImagePlane;

class MayaImagePlaneWriter : public UsdMayaPrimWriter {
public:
    MayaImagePlaneWriter(
        const MDagPath & iDag, const SdfPath& uPath,
        UsdMayaWriteJobContext& jobCtx);
    virtual ~MayaImagePlaneWriter();

    virtual void Write(const UsdTimeCode& usdTime) override;

protected:
    bool _WriteImagePlaneAttrs(
        const UsdTimeCode& usdTime, UsdGeomImagePlane& primSchema);

#ifdef GENERATE_SHADERS
    UsdPrim mTexture;
#endif
};

using MayaImagePlaneWriterPtr = std::shared_ptr<MayaImagePlaneWriter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif //_usdExport_MayaImagePlaneWriter_h_