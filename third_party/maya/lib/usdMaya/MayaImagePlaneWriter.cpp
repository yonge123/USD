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

MayaImagePlaneWriter::MayaImagePlaneWriter(const MDagPath & iDag, const SdfPath& uPath, usdWriteJobCtx& jobCtx)
    : MayaPrimWriter(iDag, uPath, jobCtx)
{
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
    if (usdTime.IsDefault() == _HasAnimCurves()) {
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
