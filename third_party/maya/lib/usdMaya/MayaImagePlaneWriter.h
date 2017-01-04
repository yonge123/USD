//
// Created by palm on 1/4/17.
//

#ifndef _usdExport_MayaImagePlaneWriter_h_
#define _usdExport_MayaImagePlaneWriter_h_

#include "usdMaya/MayaPrimWriter.h"

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

#endif //_usdExport_MayaImagePlaneWriter_h_
