#include "MayaImagePlaneWriter.h"

#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/imagePlane.h"
#include <maya/MBoundingBox.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MRenderUtil.h>

#include "writeUtil.h"

#ifdef GENERATE_SHADERS
#include <pxr/imaging/glf/glslfx.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdHydra/tokens.h>

#include <maya/MRenderUtil.h>
#endif

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

#ifdef GENERATE_SHADERS
const TfToken materialNameToken("HdMaterial");
const TfToken shaderNameToken("HdShader");
const TfToken primvarNameToken("HdPrimvar");
const TfToken textureNameToken("HdTexture");
const TfToken stToken("st");
const TfToken uvToken("uv");
const TfToken resultToken("result");
const TfToken baseColorToken("baseColor");
const TfToken colorToken("color");
#endif

}

MayaImagePlaneWriter::MayaImagePlaneWriter(const MDagPath & iDag, const SdfPath& uPath, bool instanceSource, usdWriteJobCtx& jobCtx)
    : MayaTransformWriter(iDag, uPath, instanceSource, jobCtx),
      mIsShapeAnimated(false)
{
    if (getArgs().exportAnimation) {
        MObject obj = getDagPath().node();
        mIsShapeAnimated = PxrUsdMayaUtil::isAnimated(obj);
    }

    // TODO inherit from the TransformWriter and handle it like the other shapes
    if (getArgs().mergeTransformAndShape) {
        // TODO: Replicating logic from MayaTransformWriter... and MDagPathToUsdPath...
        // combine these!
        auto hasOnlyOneShapeBelow = [&jobCtx] (const MDagPath& path) {
            auto numberOfShapesDirectlyBelow = 0u;
            path.numberOfShapesDirectlyBelow(numberOfShapesDirectlyBelow);
            if (numberOfShapesDirectlyBelow != 1) {
                return false;
            }
            const auto childCount = path.childCount();
            if (childCount == 1) {
                return true;
            }
            // Make sure that the other objects are exportable - ie, still want
            // to collapse if it has two shapes below, but one of them is an
            // intermediateObject shape
            MDagPath childDag(path);
            auto numExportableChildren = 0u;
            for (auto i = 0u; i < childCount; ++i) {
                childDag.push(path.child(i));
                if (jobCtx.needToTraverse(childDag)) {
                    ++numExportableChildren;
                    if (numExportableChildren > 1) {
                        return false;
                    }
                }
                childDag.pop();
            }
            return (numExportableChildren == 1);
        };

        // the path will often look like :
        // camera transform -> camera shape -> image plane transform -> image plane shape

        // Because we are dealing with a underworld, we have the deal with the possibility that
        // the BOTH the camera shape and image plane shape are merged into their transforms...
        // ...or only one of those are... or neither are.

        // Currently, the parent MayaTransformWriter will handle the merging of the imagePlane
        // shape with the image plane transform.  Now, we just need to worry about (possibly)
        // popping out the camera shape from our usd path...

        if (getDagPath().pathCount() > 1) {
            auto cameraXformDag = getDagPath();

            // We're in the underworld, get the first xform before the underworld (should be the
            // camera's xform)

            // First, a note about dag paths + underworlds - the length of a dag path will be
            //    mainPath.length() = sub_path_sum + mainPath.pathCount() - 1
            // where
            //    sub_path_sum = sum(mainPath.getPath(i) for i in 0<i<mainPath.pathCount())
            // Why is it not just equal to sub_path_sum? Why do we add in mainPath.pathCount?
            // Because each "root" node of each underworld counts as a node when iterating up -
            // That is, if we start with:
            //    |camXform|camShape->|imagePlaneXform|imagePlaneShape
            // Then, as we pop paths, we will get:
            //    |camXform|camShape->|imagePlaneXform
            //    |camXform|camShape->|
            //    |camXform|camShape
            //    |camXform
            // ...for a total of 5 paths. The oddity is "|camXform|camShape->|", for which there
            // will NOT be a corresponding element added to the usdPath, so we need to make
            // sure that is handled correctly!

            // First, get the number of elements in our current underworld:
            MDagPath underworldDag;
            cameraXformDag.getPath(underworldDag, cameraXformDag.pathCount() - 1);
            const auto underworldLength = underworldDag.length();

            // Then pop - note we add two:
            //    just popping underworldLength will get us to the underworld root, ie, |camXform|camShape->|
            //    popping one more will get us the camera shape, ie, |camXform|camShape
            //    popping one more will get us the camera xform, ie, |camXform
            cameraXformDag.pop(underworldLength + 2);

            // Now we test THIS to see if it is merged...
            if (hasOnlyOneShapeBelow(cameraXformDag)) {
                // Ok, cameraShape was merged... need to remove the appropriate element from our
                // usd path...

                // We use tokens from the already-created path - that way, we don't redo
                // processing done by MDagPathToUsdPath, and we avoid the hash from
                // token re-creation
                SdfPath cameraShapePath = getUsdPath();
                for (auto i = 0u; i < underworldLength - 1; ++i) {
                    cameraShapePath = cameraShapePath.GetParentPath();
                }
                auto cameraXformPath = cameraShapePath.GetParentPath();

                setUsdPath(getUsdPath().ReplacePrefix(cameraShapePath, cameraXformPath));
            }
        }
    }

    UsdGeomImagePlane primSchema =
        UsdGeomImagePlane::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    mUsdPrim = primSchema.GetPrim();
    TF_AXIOM(mUsdPrim);

#ifdef GENERATE_SHADERS
    const auto materialPath = getUsdPath().AppendChild(materialNameToken);
    auto material = UsdShadeMaterial::Define(getUsdStage(), materialPath);
    auto shader = UsdShadeShader::Define(getUsdStage(), materialPath.AppendChild(shaderNameToken));
    auto primvar = UsdShadeShader::Define(getUsdStage(), materialPath.AppendChild(primvarNameToken));
    auto texture = UsdShadeShader::Define(getUsdStage(), materialPath.AppendChild(textureNameToken));
    mTexture = texture.GetPrim();
    TF_AXIOM(mTexture);

    UsdShadeMaterialBindingAPI(mUsdPrim)
        .Bind(material);

    UsdShadeConnectableAPI::ConnectToSource(
        UsdShadeMaterial(material).CreateSurfaceOutput(GlfGLSLFXTokens->glslfx),
        UsdShadeMaterial(shader).CreateSurfaceOutput(GlfGLSLFXTokens->glslfx));

    shader.CreateOutput(UsdHydraTokens->infoFilename, SdfValueTypeNames->Asset)
        .Set(SdfAssetPath("shaders/simpleTexturedSurface.glslfx"));

    primvar.CreateIdAttr().Set(UsdHydraTokens->HwPrimvar_1);
    primvar.CreateOutput(UsdHydraTokens->infoVarname, SdfValueTypeNames->Token).Set(stToken);

    texture.CreateIdAttr().Set(UsdHydraTokens->HwUvTexture_1);
    texture.CreateOutput(UsdHydraTokens->textureMemory, SdfValueTypeNames->Float).Set(10.0f * 1024.0f * 1024.0f);

    UsdShadeConnectableAPI shaderApi(shader);
    UsdShadeConnectableAPI primvarApi(primvar);
    UsdShadeConnectableAPI textureApi(texture);

    UsdShadeConnectableAPI::ConnectToSource(
        textureApi.CreateInput(uvToken, SdfValueTypeNames->Float2),
        primvarApi.CreateOutput(resultToken, SdfValueTypeNames->Float2));

    UsdShadeConnectableAPI::ConnectToSource(
        shaderApi.CreateInput(baseColorToken, SdfValueTypeNames->Color3f),
        textureApi.CreateOutput(colorToken, SdfValueTypeNames->Color3f));
#endif
}

MayaImagePlaneWriter::~MayaImagePlaneWriter() {
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
    // Write parent class attrs
    writeTransformAttrs(usdTime, primSchema);

    if (usdTime.IsDefault() == isShapeAnimated()) {
        return true;
    }

    // Write extent
    MFnDagNode dnode(getDagPath());
    VtArray<GfVec3f> extent(2);
    auto boundingBox = dnode.boundingBox();
    extent[0] = GfVec3f(boundingBox.min().x, boundingBox.min().y, boundingBox.min().z);
    extent[1] = GfVec3f(boundingBox.max().x, boundingBox.max().y, boundingBox.max().z);
    _SetAttribute(primSchema.CreateExtentAttr(), extent, usdTime);

    const auto sizePlug = dnode.findPlug("size");
    const auto imageName = SdfAssetPath(std::string(dnode.findPlug("imageName").asString().asChar()));
    primSchema.GetFilenameAttr().Set(imageName);
#ifdef GENERATE_SHADERS
    auto imageNameExtracted = MRenderUtil::exactImagePlaneFileName(dnode.object());
    UsdShadeShader textureShader(mTexture);
    auto filenameAttr = textureShader.CreateOutput(UsdHydraTokens->infoFilename, SdfValueTypeNames->Asset);
    filenameAttr.Set(SdfAssetPath(std::string(imageNameExtracted.asChar())), usdTime);
    filenameAttr.Set(imageName);
#endif
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
    _SetAttribute(primSchema.GetFrameOffsetAttr(), dnode.findPlug("frameOffset").asInt(), usdTime);
    _SetAttribute(primSchema.GetWidthAttr(), dnode.findPlug("width").asFloat(), usdTime);
    _SetAttribute(primSchema.GetHeightAttr(), dnode.findPlug("height").asFloat(), usdTime);
    _SetAttribute(primSchema.GetAlphaGainAttr(), dnode.findPlug("alphaGain").asFloat(), usdTime);
    _SetAttribute(primSchema.GetDepthAttr(), dnode.findPlug("depth").asFloat(), usdTime);
    _SetAttribute(primSchema.GetSqueezeCorrectionAttr(), dnode.findPlug("squeezeCorrection").asFloat(), usdTime);
    const auto offsetPlug = dnode.findPlug("offset");
    _SetAttribute(primSchema.GetOffsetAttr(),
                  GfVec2f(offsetPlug.child(0).asFloat(), offsetPlug.child(1).asFloat()), usdTime);
    _SetAttribute(primSchema.GetSizeAttr(),
                  GfVec2f(sizePlug.child(0).asFloat(), sizePlug.child(1).asFloat()), usdTime);
    _SetAttribute(primSchema.GetRotateAttr(), dnode.findPlug("rotate").asFloat(), usdTime);
    const auto coveragePlug = dnode.findPlug("coverage");
    _SetAttribute(primSchema.GetCoverageAttr(),
                  GfVec2i(coveragePlug.child(0).asInt(), coveragePlug.child(1).asInt()), usdTime);
    const auto coverageOriginPlug = dnode.findPlug("coverageOrigin");
    _SetAttribute(primSchema.GetCoverageOriginAttr(),
                  GfVec2i(coverageOriginPlug.child(0).asInt(), coverageOriginPlug.child(1).asInt()), usdTime);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
