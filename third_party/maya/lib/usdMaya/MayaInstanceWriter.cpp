#include "usdMaya/MayaInstanceWriter.h"

#include "pxr/usd/usd/inherits.h"

#include <maya/MDagPathArray.h>

PXR_NAMESPACE_OPEN_SCOPE

MayaInstanceWriter::MayaInstanceWriter(MDagPath& iDag, UsdStageRefPtr stage, const JobExportArgs& iArgs,
                                       const PxrUsdMayaUtil::MDagPathMap<SdfPath>::Type* pathMapPtr)
    : MayaTransformWriter(iDag, stage, iArgs),
      mPathMap(pathMapPtr)
{
}

void MayaInstanceWriter::write(const UsdTimeCode& usdTime) {
    if (usdTime.IsDefault()) {
        MDagPathArray allInstances;
        MDagPath::getAllPathsTo(getDagPath().node(), allInstances);
        if (allInstances.length() > 0) {
            allInstances[0].pop();
            const auto it = mPathMap->find(allInstances[0]);
            if (it != mPathMap->end()) {
                mUsdPrim.GetInherits().AppendInherit(it->second);
                mUsdPrim.SetInstanceable(true);
            }
        }
    }
    MayaTransformWriter::write(usdTime);
}

PXR_NAMESPACE_CLOSE_SCOPE
