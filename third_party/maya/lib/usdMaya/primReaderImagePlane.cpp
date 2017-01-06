#include "usdMaya/primReaderRegistry.h"
#include "usdMaya/translatorImagePlane.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/imagePlane.h"
#include "pxr/usd/usdGeom/camera.h"

#include <maya/MObject.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>

PXRUSDMAYA_DEFINE_READER(UsdGeomImagePlane, args, context)
{
    const UsdPrim& usdPrim = args.GetUsdPrim();
    const auto& parent = usdPrim.GetParent();
    const auto& grandParent = usdPrim.GetParent();
    MObject parentNode = MObject::kNullObj;
    bool isCompacted = false;
    // We want to call the imageplane function on a camera shape always, because that's going to be the
    // root of the underworld.
    // Transforms were compacted, and we need to lookup the camera
    if (parent && parent.IsA<UsdGeomCamera>()) {
        MObject parentTransform = context->GetMayaNode(parent.GetPath(), true);
        MStatus status;
        MFnDagNode transformNode(parentTransform, &status);
        if (!status) { return false; }

        MDagPath shapePath;
        transformNode.getPath(shapePath);
        // If there are multiple cameras, we are f'ed, because there is no way to know the actual camera parent yet.
        status = shapePath.extendToShapeDirectlyBelow(0);
        if (!status) { return false; }

        parentNode = shapePath.node();
        isCompacted = true;
    // No they were not, this will be equal to the camera shape path
    } else if (grandParent && grandParent.IsA<UsdGeomCamera>()) {
        parentNode = context->GetMayaNode(grandParent.GetPath(), true);
    }
    if (parentNode == MObject::kNullObj || !parentNode.hasFn(MFn::kCamera)) {
        return false;
    } else {
        return PxrUsdMayaTranslatorImagePlane::Read(
            UsdGeomImagePlane(usdPrim),
            parentNode,
            args,
            context,
            isCompacted
        );
    }
}
