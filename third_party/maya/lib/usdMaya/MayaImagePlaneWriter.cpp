#include "usdMaya/MayaImagePlaneWriter.h"

#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/imagePlane.h>
#include <pxr/base/gf/range3f.h>
#include <maya/MBoundingBox.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MRenderUtil.h>

#include "usdMaya/adaptor.h"
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/usdWriteJobCtx.h"
#include "usdMaya/writeUtil.h"

#ifdef GENERATE_SHADERS
#include <pxr/imaging/glf/glslfx.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdHydra/tokens.h>
#include <pxr/usd/usdGeom/camera.h>

#include <maya/MRenderUtil.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_WRITER(imagePlane, MayaImagePlaneWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(imagePlane, UsdGeomImagePlane);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((defaultOutputName, "out"))
#ifdef GENERATE_SHADERS
    ((materialName, "HdMaterial"))
    ((shaderName, "HdShader"))
    ((primvarName, "HdPrimvar"))
    ((textureName, "HdTexture"))
    (st)
    (uv)
    (result)
    (baseColor)
    (color)
#endif
);

MayaImagePlaneWriter::MayaImagePlaneWriter(const MDagPath & iDag, const SdfPath& uPath, bool instanceSource, usdWriteJobCtx& jobCtx)
    : MayaTransformWriter(iDag, uPath, instanceSource, jobCtx)
{
    if (_GetExportArgs().mergeTransformAndShape) {
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

        if (GetDagPath().pathCount() > 1) {
            auto cameraXformDag = GetDagPath();

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
                SdfPath cameraShapePath = GetUsdPath();
                for (auto i = 0u; i < underworldLength - 1; ++i) {
                    cameraShapePath = cameraShapePath.GetParentPath();
                }
                auto cameraXformPath = cameraShapePath.GetParentPath();

                _SetUsdPath(GetUsdPath().ReplacePrefix(cameraShapePath, cameraXformPath));
            }
        }
    }

    auto primSchema =
        UsdGeomImagePlane::Define(GetUsdStage(), GetUsdPath());
    TF_AXIOM(primSchema);
    _usdPrim = primSchema.GetPrim();
    TF_AXIOM(_usdPrim);

#ifdef GENERATE_SHADERS
    const auto materialPath = GetUsdPath().AppendChild(_tokens->materialName);
    auto material = UsdShadeMaterial::Define(GetUsdStage(), materialPath);
    auto shader = UsdShadeShader::Define(GetUsdStage(), materialPath.AppendChild(_tokens->shaderName));
    auto primvar = UsdShadeShader::Define(GetUsdStage(), materialPath.AppendChild(_tokens->primvarName));
    auto texture = UsdShadeShader::Define(GetUsdStage(), materialPath.AppendChild(_tokens->textureName));
    mTexture = texture.GetPrim();
    TF_AXIOM(mTexture);

    UsdShadeMaterialBindingAPI(_usdPrim)
        .Bind(material);

    UsdShadeConnectableAPI::ConnectToSource(
        UsdShadeMaterial(material).CreateSurfaceOutput(GlfGLSLFXTokens->glslfx),
        UsdShadeMaterial(shader).CreateOutput(_tokens->defaultOutputName, SdfValueTypeNames->Token));

    shader.GetPrim()
        .CreateAttribute(UsdHydraTokens->infoFilename, SdfValueTypeNames->Asset, SdfVariabilityUniform)
        .Set(SdfAssetPath("shaders/simpleTexturedSurface.glslfx"));

    primvar.CreateIdAttr().Set(UsdHydraTokens->HwPrimvar_1);
    primvar.GetPrim()
        .CreateAttribute(UsdHydraTokens->infoVarname, SdfValueTypeNames->Token, SdfVariabilityUniform)
        .Set(_tokens->st);

    texture.CreateIdAttr().Set(UsdHydraTokens->HwUvTexture_1);
    texture.GetPrim()
        .CreateAttribute(UsdHydraTokens->textureMemory, SdfValueTypeNames->Float, SdfVariabilityUniform)
        .Set(10.0f * 1024.0f * 1024.0f);

    UsdShadeConnectableAPI shaderApi(shader);
    UsdShadeConnectableAPI primvarApi(primvar);
    UsdShadeConnectableAPI textureApi(texture);

    UsdShadeConnectableAPI::ConnectToSource(
        textureApi.CreateInput(_tokens->uv, SdfValueTypeNames->Float2),
        primvarApi.CreateOutput(_tokens->result, SdfValueTypeNames->Float2));

    UsdShadeConnectableAPI::ConnectToSource(
        shaderApi.CreateInput(_tokens->baseColor, SdfValueTypeNames->Color4f),
        textureApi.CreateOutput(_tokens->color, SdfValueTypeNames->Color4f));

    for (auto pt = GetUsdPath(); !pt.IsEmpty(); pt = pt.GetParentPath()) {
        auto pr = GetUsdStage()->GetPrimAtPath(pt);
        if (pr && pr.IsA<UsdGeomCamera>()) {
            primSchema.CreateCameraRel().AddTarget(pt);
            break;
        }
    }
#endif
}

MayaImagePlaneWriter::~MayaImagePlaneWriter() {
}

void MayaImagePlaneWriter::Write(const UsdTimeCode& usdTime) {
    UsdGeomImagePlane primSchema(_usdPrim);

    // Write the attrs
    _WriteImagePlaneAttrs(usdTime, primSchema);
}

bool MayaImagePlaneWriter::_WriteImagePlaneAttrs(const UsdTimeCode& usdTime, UsdGeomImagePlane& primSchema) {
    // Write parent class attrs
    _WriteXformableAttrs(usdTime, primSchema);

    if (usdTime.IsDefault() == _IsShapeAnimated()) {
        return true;
    }

    // Write extent, just the default for now. It should be setup in the adapter for drawing.
    MFnDagNode dnode(GetDagPath());
    
    auto imageNameExtracted = MRenderUtil::exactImagePlaneFileName(dnode.object());
    const SdfAssetPath imageNameExtractedPath(std::string(imageNameExtracted.asChar()));
    const auto sizePlug = dnode.findPlug("size");
    const auto imageName = SdfAssetPath(std::string(dnode.findPlug("imageName").asString().asChar()));
    primSchema.GetFilenameAttr().Set(imageName);
    primSchema.GetFilenameAttr().Set(imageNameExtractedPath, usdTime);
#ifdef GENERATE_SHADERS
    UsdShadeShader textureShader(mTexture);
    auto filenameAttr = textureShader.GetPrim()
        .CreateAttribute(UsdHydraTokens->infoFilename, SdfValueTypeNames->Asset, SdfVariabilityVarying);
    filenameAttr.Set(imageNameExtractedPath, usdTime);
    filenameAttr.Set(imageName);
#endif
    const auto fit = dnode.findPlug("fit").asShort();
    if (fit == UsdGeomImagePlane::FIT_BEST) {
        primSchema.GetFitAttr().Set(UsdGeomImagePlaneFitTokens->best);
    } else if (fit == UsdGeomImagePlane::FIT_FILL) {
        primSchema.GetFitAttr().Set(UsdGeomImagePlaneFitTokens->fill);
    } else if (fit == UsdGeomImagePlane::FIT_HORIZONTAL) {
        primSchema.GetFitAttr().Set(UsdGeomImagePlaneFitTokens->horizontal);
    } else if (fit == UsdGeomImagePlane::FIT_VERTICAL) {
        primSchema.GetFitAttr().Set(UsdGeomImagePlaneFitTokens->vertical);
    } else if (fit == UsdGeomImagePlane::FIT_TO_SIZE) {
        primSchema.GetFitAttr().Set(UsdGeomImagePlaneFitTokens->toSize);
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
    VtVec3fArray positions;
    primSchema.CalculateGeometryForViewport(&positions, nullptr, usdTime);
    GfRange3f extent;
    for (const auto& vertex: positions) {
        extent.ExtendBy(vertex);
    }
    VtArray<GfVec3f> extents(2);
    extents[0] = extent.GetMin();
    extents[1] = extent.GetMax();
    _SetAttribute(primSchema.CreateExtentAttr(), extents, usdTime);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
