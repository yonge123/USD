#ifndef _usdExport_MayaInstanceWriter_h_
#define _usdExport_MayaInstanceWriter_h_

#include "pxr/pxr.h"

#include "usdMaya/MayaTransformWriter.h"

PXR_NAMESPACE_OPEN_SCOPE

class MayaInstanceWriter : public MayaTransformWriter {
public:
    MayaInstanceWriter(MDagPath& iDag, UsdStageRefPtr stage, const JobExportArgs& iArgs,
                       const PxrUsdMayaUtil::MDagPathMap<SdfPath>::Type* pathMapPtr);
    virtual void write(const UsdTimeCode& usdTime);

private:
    const PxrUsdMayaUtil::MDagPathMap<SdfPath>::Type* mPathMap;
};

typedef shared_ptr<MayaInstanceWriter> MayaInstanceWriterPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
