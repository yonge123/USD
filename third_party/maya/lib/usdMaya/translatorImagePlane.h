#ifndef PXRUSDMAYA_TRANSLATOR_IMAGEPLANE_H
#define PXRUSDMAYA_TRANSLATOR_IMAGEPLANE_H

#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"

#include "pxr/usd/usdGeom/imagePlane.h"

#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Provides helper functions for translating to/from UsdGeomImagePlane
struct PxrUsdMayaTranslatorImagePlane
{
    /// Reads a UsdGeomImagePlane \p usdCamera from USD and creates a Maya
    /// MFnImagePlane under \p parentNode.
    static bool Read(
        const UsdGeomImagePlane& usdImagePlane,
        MObject parentNode,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context,
        bool isCompacted);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_TRANSLATOR_IMAGEPLANE_H