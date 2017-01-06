#include "usdMaya/primReaderRegistry.h"
#include "usdMaya/translatorImagePlane.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/imagePlane.h"

#include <maya/MObject.h>

PXRUSDMAYA_DEFINE_READER(UsdGeomImagePlane, args, context)
{
    const UsdPrim& usdPrim = args.GetUsdPrim();
    MObject parentNode = context->GetMayaNode(usdPrim.GetPath().GetParentPath(), true);
    return PxrUsdMayaTranslatorImagePlane::Read(
        UsdGeomImagePlane(usdPrim),
        parentNode,
        args,
        context);
}
