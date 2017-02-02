#include "usdMaya/VdbVisualizerWriter.h"
#include "pxr/usd/usdAi/aiVolume.h"

PXR_NAMESPACE_OPEN_SCOPE

VdbVisualizerWriter::VdbVisualizerWriter(MDagPath& iDag, UsdStageRefPtr stage, const JobExportArgs& iArgs) :
    MayaTransformWriter(iDag, stage, iArgs, false) {
    UsdAiVolume primSchema =
        UsdAiVolume::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    mUsdPrim = primSchema.GetPrim();
    TF_AXIOM(mUsdPrim);
}

void VdbVisualizerWriter::write(const UsdTimeCode& usdTime) {
    UsdAiVolume primSchema(mUsdPrim);
    writeTransformAttrs(usdTime, primSchema);

    if (usdTime.IsDefault() == isShapeAnimated()) {
        return;
    }
}
