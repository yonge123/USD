#include "MayaImagePlaneWriter.h"

#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/imagePlane.h"
#include <maya/MFnDependencyNode.h>

#include "writeUtil.h"

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

MayaImagePlaneWriter::MayaImagePlaneWriter(const MDagPath & iDag, const SdfPath& uPath, bool instanceSource, usdWriteJobCtx& jobCtx)
    : MayaPrimWriter(iDag, uPath, jobCtx), mIsShapeAnimated(false) {
    if (getArgs().exportAnimation) {
        MObject obj = getDagPath().node();
        mIsShapeAnimated = PxrUsdMayaUtil::isAnimated(obj);
    }

    if (getArgs().mergeTransformAndShape) {
        // the path will always look like :
        // camera transform -> camera shape -> image plane transform -> image plane shape
        // So first we pop the image plane shape if possible,
        // then we are removing the camera shape
        auto imagePlaneXform = iDag;
        imagePlaneXform.pop();
        auto cameraXform = imagePlaneXform;
        cameraXform.pop();

        unsigned int imagePlaneShapes = 0;
        unsigned int cameraShapes = 0;
        imagePlaneXform.numberOfShapesDirectlyBelow(imagePlaneShapes);
        cameraXform.numberOfShapesDirectlyBelow(cameraShapes);

        // Use tokens from the already-created path - that way, we don't redo
        // processing done by MDagPathToUsdPath, and we avoid the hash from
        // token re-creation
        if (cameraShapes == 1 || imagePlaneShapes == 1) {
            SdfPath usdPath;
            if (cameraShapes == 1) {
                // Get the cameraShape path, then swap it out for the
                // imagePlaneXform, effectively removing it
                auto impagePlaneXformPath = getUsdPath().GetParentPath();
                usdPath = impagePlaneXformPath.GetParentPath().ReplaceName(
                        impagePlaneXformPath.GetElementToken());
            }
            else {
                // We just use the camera shape path
                usdPath = getUsdPath().GetParentPath().GetParentPath();
            }

            if (imagePlaneShapes == 1) {
                usdPath.AppendChild(getUsdPath().GetParentPath().GetElementToken());
            }
            else {
                usdPath = usdPath
                        .AppendChild(getUsdPath().GetParentPath().GetElementToken())
                        .AppendChild(getUsdPath().GetElementToken());
            }
            setUsdPath(usdPath);
        }
    }

    UsdGeomImagePlane primSchema =
        UsdGeomImagePlane::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    mUsdPrim = primSchema.GetPrim();
    TF_AXIOM(mUsdPrim);
}

MayaImagePlaneWriter::~MayaImagePlaneWriter() {
}

// virtual override
void MayaImagePlaneWriter::postExport() {
    UsdGeomImagePlane primSchema(mUsdPrim);

    if (primSchema) {
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetFilenameAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetUseFrameExtensionAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetFrameOffsetAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetWidthAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetHeightAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetAlphaGainAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetAlphaGainAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetDepthAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetSqueezeCorrectionAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetOffsetAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetSizeAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetRotateAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetCoverageAttr(), UsdInterpolationTypeHeld);
        PxrUsdMayaWriteUtil::CleanupAttributeKeys(primSchema.GetCoverageOriginAttr(), UsdInterpolationTypeHeld);
    }
}

void MayaImagePlaneWriter::write(const UsdTimeCode& usdTime) {
    UsdGeomImagePlane primSchema(mUsdPrim);

    // Write the attrs
    writeImagePlaneAttrs(usdTime, primSchema);
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
