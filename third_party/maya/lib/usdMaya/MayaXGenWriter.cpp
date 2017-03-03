#include "usdMaya/MayaXGenWriter.h"

MayaXGenWriter::MayaXGenWriter(const MDagPath & iDag,
               const SdfPath& uPath,
               bool instanceSource,
               usdWriteJobCtx& job) : MayaTransformWriter(iDag, uPath, instanceSource, job) {

}

MayaXGenWriter::~MayaXGenWriter() {

}

void MayaXGenWriter::write(const UsdTimeCode& usdTime){

}
