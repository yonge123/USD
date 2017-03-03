#ifndef _usdExport_MayaXGenWriter_h_
#define _usdExport_MayaXGenWriter_h_

#include "pxr/pxr.h"
#include "usdMaya/MayaTransformWriter.h"

PXR_NAMESPACE_OPEN_SCOPE

class MayaXGenWriter : public MayaTransformWriter {
public:
    MayaXGenWriter(const MDagPath & iDag,
                        const SdfPath& uPath,
                        bool instanceSource,
                        usdWriteJobCtx& job);
    virtual ~MayaXGenWriter();

    virtual void write(const UsdTimeCode& usdTime) override;
};

typedef std::shared_ptr<MayaXGenWriter> MayaXGenWriterPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
