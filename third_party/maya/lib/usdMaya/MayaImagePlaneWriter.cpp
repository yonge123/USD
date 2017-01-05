#include "MayaImagePlaneWriter.h"

#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/imagePlane.h"
#include <maya/MFnDependencyNode.h>

namespace {
    enum {
        IMAGE_PLANE_FIT_FILL,
        IMAGE_PLANE_FIT_BEST,
        IMAGE_PLANE_FIT_HORIZONTAL,
        IMAGE_PLANE_FIT_VERTICAL,
        IMAGE_PLANE_FIT_TO_SIZE
    };

    const TfToken image_plane_fill("fill");
    const TfToken image_plane_best("best");
    const TfToken image_plane_horizontal("horizontal");
    const TfToken image_plane_vertical("vertical");
    const TfToken image_plane_to_size("to size");
}

MayaImagePlaneWriter::MayaImagePlaneWriter(MDagPath& iDag, UsdStageRefPtr stage, const JobExportArgs& iArgs)
    : MayaPrimWriter(iDag, stage, iArgs), mIsShapeAnimated(false) {
    if (iArgs.exportAnimation) {
        MObject obj = getDagPath().node();
        mIsShapeAnimated = PxrUsdMayaUtil::isAnimated(obj);
    }
}

UsdPrim MayaImagePlaneWriter::write(const UsdTimeCode& usdTime) {
    UsdGeomImagePlane primSchema =
        UsdGeomImagePlane::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    UsdPrim prim = primSchema.GetPrim();
    TF_AXIOM(prim);

    // Write the attrs
    writeImagePlaneAttrs(usdTime, primSchema);
    return prim;
}

bool MayaImagePlaneWriter::isShapeAnimated() const {
    return mIsShapeAnimated;
}

bool MayaImagePlaneWriter::writeImagePlaneAttrs(const UsdTimeCode& usdTime, UsdGeomImagePlane& primSchema) {
    if (usdTime.IsDefault() == isShapeAnimated()) {
        return true;
    }

    MFnDependencyNode dnode(getDagPath().node());
    const auto sizePlug = dnode.findPlug("size");
    primSchema.GetFilenameAttr().Set(SdfAssetPath(std::string(dnode.findPlug("imageName").asString().asChar())));
    const auto fit = dnode.findPlug("fit").asShort();
    if (fit == IMAGE_PLANE_FIT_BEST) {
        primSchema.GetFitAttr().Set(image_plane_best, usdTime);
    } else if (fit == IMAGE_PLANE_FIT_FILL) {
        primSchema.GetFitAttr().Set(image_plane_fill, usdTime);
    } else if (fit == IMAGE_PLANE_FIT_HORIZONTAL) {
        primSchema.GetFitAttr().Set(image_plane_horizontal, usdTime);
    } else if (fit == IMAGE_PLANE_FIT_VERTICAL) {
        primSchema.GetFitAttr().Set(image_plane_vertical, usdTime);
    } else if (fit == IMAGE_PLANE_FIT_TO_SIZE) {
        primSchema.GetFitAttr().Set(image_plane_to_size, usdTime);
    }
    const auto offsetPlug = dnode.findPlug("offset");
    primSchema.GetOffsetAttr().Set(GfVec2f(offsetPlug.child(0).asFloat(), offsetPlug.child(1).asFloat()), usdTime);
    primSchema.GetSizeAttr().Set(GfVec2f(sizePlug.child(0).asFloat(), sizePlug.child(1).asFloat()), usdTime);
    primSchema.GetRotateAttr().Set(dnode.findPlug("rotate").asFloat(), usdTime);
    const auto coveragePlug = dnode.findPlug("coverage");
    primSchema.GetCoverageAttr().Set(GfVec2i(coveragePlug.child(0).asInt(), coveragePlug.child(1).asInt()), usdTime);
    const auto coverageOriginPlug = dnode.findPlug("coverageOrigin");
    primSchema.GetCoverageOriginAttr().Set(GfVec2i(coverageOriginPlug.child(0).asInt(), coverageOriginPlug.child(1).asInt()), usdTime);
    return true;
}
