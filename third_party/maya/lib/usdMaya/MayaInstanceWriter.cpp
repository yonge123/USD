#include "usdMaya/MayaInstanceWriter.h"

PXR_NAMESPACE_OPEN_SCOPE

MayaInstanceWriter::MayaInstanceWriter(MDagPath& iDag, UsdStageRefPtr stage, const JobExportArgs& iArgs)
    : MayaTransformWriter(iDag, stage, iArgs)
{

}

void MayaInstanceWriter::write(const UsdTimeCode& usdTime) {
    if (usdTime.IsDefault()) {
        // handling inherits information
    }
    MayaTransformWriter::write(usdTime);
}

PXR_NAMESPACE_CLOSE_SCOPE
