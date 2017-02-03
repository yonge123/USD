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
    MayaImagePlaneWriter(MDagPath& iDag, UsdStageRefPtr stage, const JobExportArgs& iArgs);
    virtual ~MayaImagePlaneWriter() {};
    virtual UsdPrim write(const UsdTimeCode& usdTime);
    virtual bool isShapeAnimated() const;
protected:
    bool writeImagePlaneAttrs(const UsdTimeCode& usdTime, UsdGeomImagePlane& primSchema);

    bool mIsShapeAnimated;
};

typedef shared_ptr<MayaImagePlaneWriter> MayaImagePlaneWriterPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif //_usdExport_MayaImagePlaneWriter_h_
