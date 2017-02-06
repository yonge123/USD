#ifndef _usdExport_MayaInstanceWriter_h_
#define _usdExport_MayaInstanceWriter_h_

#include "pxr/pxr.h"

#include "usdMaya/MayaTransformWriter.h"

PXR_NAMESPACE_OPEN_SCOPE

class MayaInstanceWriter : public MayaTransformWriter {
public:
    MayaInstanceWriter(MDagPath& iDag, UsdStageRefPtr stage, const JobExportArgs& iArgs);
    virtual void write(const UsdTimeCode& usdTime);
};

typedef shared_ptr<MayaInstanceWriter> MayaInstanceWriterPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
