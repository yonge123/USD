#include "MayaImagePlaneWriter.h"

#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/imagePlane.h"
#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE


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

    if (iArgs.mergeTransformAndShape) {
        // the path will always look like :
        // camera transform -> camera shape -> image plane transform -> image plane shape
        // So first we pop the image plane shape if possible,
        // then we are removing the camera shape
        auto iDagCopy = iDag;
        std::string shapeName = MFnDependencyNode(iDagCopy.node()).name().asChar();
        iDagCopy.pop();
        std::string transformName = MFnDependencyNode(iDagCopy.node()).name().asChar();
        unsigned int numberOfShapesDirectlyBelow = 0;
        iDagCopy.numberOfShapesDirectlyBelow(numberOfShapesDirectlyBelow);
        auto usdPath = getUsdPath().GetParentPath().GetParentPath().GetParentPath();
        if (numberOfShapesDirectlyBelow == 1) {
            usdPath = usdPath.AppendChild(TfToken(transformName));
        } else {
            usdPath = usdPath.AppendChild(TfToken(transformName)).AppendChild(TfToken(shapeName));
        }
        setUsdPath(usdPath);
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
        primSchema.GetFitAttr().Set(image_plane_best);
    } else if (fit == IMAGE_PLANE_FIT_FILL) {
        primSchema.GetFitAttr().Set(image_plane_fill);
    } else if (fit == IMAGE_PLANE_FIT_HORIZONTAL) {
        primSchema.GetFitAttr().Set(image_plane_horizontal);
    } else if (fit == IMAGE_PLANE_FIT_VERTICAL) {
        primSchema.GetFitAttr().Set(image_plane_vertical);
    } else if (fit == IMAGE_PLANE_FIT_TO_SIZE) {
        primSchema.GetFitAttr().Set(image_plane_to_size);
    }
    primSchema.GetUseFrameExtensionAttr().Set(dnode.findPlug("useFrameExtension").asBool());
    primSchema.GetFrameOffsetAttr().Set(dnode.findPlug("frameOffset").asInt(), usdTime);
    primSchema.GetFrameCacheAttr().Set(dnode.findPlug("frameCache").asInt());
    primSchema.GetWidthAttr().Set(dnode.findPlug("width").asFloat(), usdTime);
    primSchema.GetHeightAttr().Set(dnode.findPlug("height").asFloat(), usdTime);
    primSchema.GetAlphaGainAttr().Set(dnode.findPlug("alphaGain").asFloat(), usdTime);
    primSchema.GetDepthAttr().Set(dnode.findPlug("depth").asFloat(), usdTime);
    primSchema.GetSqueezeCorrectionAttr().Set(dnode.findPlug("squeezeCorrection").asFloat(), usdTime);
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

PXR_NAMESPACE_CLOSE_SCOPE
