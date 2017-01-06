#include "translatorImagePlane.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnCamera.h>

bool PxrUsdMayaTranslatorImagePlane::Read(
    const UsdGeomImagePlane& usdImagePlane,
    MObject parentNode,
    const PxrUsdMayaPrimReaderArgs& args,
    PxrUsdMayaPrimReaderContext* context) {

    return true;
}
