#include "translatorImagePlane.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnCamera.h>
#include <maya/MDagModifier.h>
#include <maya/MPlug.h>
#include <maya/MTimeArray.h>
#include <maya/MFnAnimCurve.h>

#include "usdMaya/translatorUtil.h"
#include "MayaImagePlaneWriter.h"

namespace {
    // From the camera translator code, cleaned the code up a bit
    const TfToken sizeToken("size");
    const TfToken offsetToken("offset");
    const TfToken rotateToken("rotate");
    const TfToken coverageToken("coverage");
    const TfToken coverageOriginToken("coverageOrigin");

    const TfType floatType = TfType::Find<float>();
    const TfType float2Type = TfType::Find<GfVec2f>();
    const TfType int2Type = TfType::Find<GfVec2i>();

    bool
    resize_arrays(
        const UsdAttribute& usdAttr,
        const PxrUsdMayaPrimReaderArgs& args,
        std::vector<double>& timeSamples,
        size_t array_count,
        MTimeArray* timeArray,
        MDoubleArray* valueArray)
    {
        if (!PxrUsdMayaTranslatorUtil::GetTimeSamples(usdAttr, args,
                                                      &timeSamples)) {
            return false;
        }

        size_t numTimeSamples = timeSamples.size();
        if (numTimeSamples < 1) {
            return false;
        }

        for (auto i = 0; i < array_count; ++i) {
            timeArray[i].setLength(static_cast<unsigned int>(numTimeSamples));
            valueArray[i].setLength(static_cast<unsigned int>(numTimeSamples));
        }

        return true;
    }

    bool
    get_time_and_value_for_float(
        const UsdAttribute& usdAttr,
        const PxrUsdMayaPrimReaderArgs& args,
        MTimeArray* timeArray,
        MDoubleArray* valueArray)
    {
        std::vector<double> timeSamples;
        if (!resize_arrays(usdAttr, args, timeSamples,
                           1, timeArray, valueArray)) {
            return false;
        }

        size_t numTimeSamples = timeSamples.size();

        for (size_t i = 0; i < numTimeSamples; ++i) {
            const double timeSample = timeSamples[i];
            float attrValue;
            if (!usdAttr.Get(&attrValue, timeSample)) {
                return false;
            }
            timeArray->set(MTime(timeSample), static_cast<unsigned int>(i));
            valueArray->set(attrValue, static_cast<unsigned int>(i));
        }

        return true;
    }

    bool
    get_time_and_value_for_float2(
        const UsdAttribute& usdAttr,
        const PxrUsdMayaPrimReaderArgs& args,
        MTimeArray* timeArray,
        MDoubleArray* valueArray)
    {
        std::vector<double> timeSamples;
        if (!resize_arrays(usdAttr, args, timeSamples, 2, timeArray, valueArray)) {
            return false;
        }

        const auto numTimeSamples = timeSamples.size();

        for (auto i = 0; i < numTimeSamples; ++i) {
            const auto timeSample = timeSamples[i];
            GfVec2f attrValue;
            if (!usdAttr.Get(&attrValue, timeSample)) {
                return false;
            }
            timeArray[0].set(MTime(timeSample), static_cast<unsigned int>(i));
            timeArray[1].set(MTime(timeSample), static_cast<unsigned int>(i));
            valueArray[0].set(attrValue[0], static_cast<unsigned int>(i));
            valueArray[1].set(attrValue[1], static_cast<unsigned int>(i));
        }

        return true;
    }

    bool
    get_time_and_value_for_int2(
        const UsdAttribute& usdAttr,
        const PxrUsdMayaPrimReaderArgs& args,
        MTimeArray* timeArray,
        MDoubleArray* valueArray)
    {
        std::vector<double> timeSamples;
        if (!resize_arrays(usdAttr, args, timeSamples, 2, timeArray, valueArray)) {
            return false;
        }

        const auto numTimeSamples = timeSamples.size();

        for (auto i = 0; i < numTimeSamples; ++i) {
            const auto timeSample = timeSamples[i];
            GfVec2i attrValue;
            if (!usdAttr.Get(&attrValue, timeSample)) {
                return false;
            }
            timeArray[0].set(MTime(timeSample), static_cast<unsigned int>(i));
            timeArray[1].set(MTime(timeSample), static_cast<unsigned int>(i));
            valueArray[0].set(static_cast<double>(attrValue[0]), static_cast<unsigned int>(i));
            valueArray[1].set(static_cast<double>(attrValue[1]), static_cast<unsigned int>(i));
        }

        return true;
    }

    bool
    create_anim_curve_plug(
        MPlug& plug,
        MTimeArray& timeArray,
        MDoubleArray& valueArray,
        PxrUsdMayaPrimReaderContext* context)
    {
        MFnAnimCurve animFn;
        MStatus status;
        MObject animObj = animFn.create(plug, NULL, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = animFn.addKeys(&timeArray, &valueArray);
        CHECK_MSTATUS_AND_RETURN(status, false);

        if (context) {
            // used for undo/redo
            context->RegisterNewMayaNode(animFn.name().asChar(), animObj);
        }

        return true;
    }

    bool translate_animated_float(MPlug& plug, const UsdAttribute& usdAttr,
                                  const PxrUsdMayaPrimReaderArgs& args,
                                  PxrUsdMayaPrimReaderContext* context)
    {
        if (!args.GetReadAnimData()) {
            return false;
        }

        MTimeArray timeArray;
        MDoubleArray valueArray;
        if (!get_time_and_value_for_float(usdAttr, args, &timeArray, &valueArray)) {
            return false;
        }

        return create_anim_curve_plug(plug, timeArray, valueArray, context);
    }

    bool translate_float_attribute(MPlug& plug, const UsdAttribute& usdAttr)
    {
        UsdTimeCode timeCode = UsdTimeCode::EarliestTime();
        auto attrValue = 0.0f;
        usdAttr.Get(&attrValue, timeCode);
        auto status = plug.setFloat(attrValue);
        return status;
    }

    bool translate_animated_float2(MPlug& plug, const UsdAttribute& usdAttr,
                                  const PxrUsdMayaPrimReaderArgs& args,
                                  PxrUsdMayaPrimReaderContext* context)
    {
        if (!args.GetReadAnimData()) {
            return false;
        }

        MTimeArray time_array[2];
        MDoubleArray value_array[2];
        if (!get_time_and_value_for_float2(usdAttr, args,
                                           time_array, value_array)) {
            return false;
        }

        auto child_0 = plug.child(0);
        auto child_1 = plug.child(1);

        return create_anim_curve_plug(child_0, time_array[0], value_array[0], context) &&
               create_anim_curve_plug(child_1, time_array[1], value_array[1], context);
    }

    bool translate_float2_attribute(MPlug& plug, const UsdAttribute& usdAttr)
    {
        UsdTimeCode timeCode = UsdTimeCode::EarliestTime();
        GfVec2f attrValue(0.0f, 0.0f);
        usdAttr.Get(&attrValue, timeCode);
        auto status = plug.child(0).setFloat(attrValue[0]);
        CHECK_MSTATUS_AND_RETURN(status, false);
        status = plug.child(1).setFloat(attrValue[1]);
        return status;
    }

    bool translate_animated_int2(MPlug& plug, const UsdAttribute& usdAttr,
                                  const PxrUsdMayaPrimReaderArgs& args,
                                  PxrUsdMayaPrimReaderContext* context)
    {
        if (!args.GetReadAnimData()) {
            return false;
        }

        MTimeArray time_array[2];
        MDoubleArray value_array[2];
        if (!get_time_and_value_for_int2(usdAttr, args,
                                           time_array, value_array)) {
            return false;
        }

        auto child_0 = plug.child(0);
        auto child_1 = plug.child(1);

        return create_anim_curve_plug(child_0, time_array[0], value_array[0], context) &&
               create_anim_curve_plug(child_1, time_array[1], value_array[1], context);
    }

    bool translate_int2_attribute(MPlug& plug, const UsdAttribute& usdAttr)
    {
        UsdTimeCode timeCode = UsdTimeCode::EarliestTime();
        GfVec2i attrValue(0, 0);
        usdAttr.Get(&attrValue, timeCode);
        auto status = plug.child(0).setInt(attrValue[0]);
        CHECK_MSTATUS_AND_RETURN(status, false);
        status = plug.child(1).setInt(attrValue[1]);
        return status;
    }

    bool
    TranslateUsdAttributeToPlug(
        const UsdAttribute& usdAttr,
        const MFnDependencyNode& depNode,
        TfToken plugName,
        const PxrUsdMayaPrimReaderArgs& args,
        PxrUsdMayaPrimReaderContext* context)
    {
        MStatus status;

        auto plug = depNode.findPlug(plugName.GetText(), true, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        const auto typeName = usdAttr.GetTypeName().GetType();
        if (typeName == floatType) {
            if (!translate_animated_float(plug, usdAttr, args, context)) {
                return translate_float_attribute(plug, usdAttr);
            }
        } else if (typeName == float2Type) {
            if (!translate_animated_float2(plug, usdAttr, args, context)) {
                return translate_float2_attribute(plug, usdAttr);
            }
        } else if  (typeName == int2Type) {
            if (!translate_animated_int2(plug, usdAttr, args, context)) {
                return translate_int2_attribute(plug, usdAttr);
            }
        }

        return true;
    }
}

bool PxrUsdMayaTranslatorImagePlane::Read(
    const UsdGeomImagePlane& usdImagePlane,
    MObject parentNode,
    const PxrUsdMayaPrimReaderArgs& args,
    PxrUsdMayaPrimReaderContext* context,
    bool isCompacted
) {
    MFnDependencyNode cameraNode(parentNode);
    MStatus status;

    MDagModifier dagMod;
    MObject transformObj = dagMod.createNode("transform", parentNode, &status);
    if (!status) { TF_WARN("Error creating transform for image plane!"); return false; }
    MObject imagePlaneObj = dagMod.createNode("imagePlane", transformObj, &status);
    if (!status) { TF_WARN("Error creating transform for image plane!"); return false; }
    status = dagMod.doIt();
    if (!status) { TF_WARN("Error creating transform for image plane!"); return false; }

    const auto usdImagePlanePrim = usdImagePlane.GetPrim();
    MFnDependencyNode transformNode(transformObj);
    MFnDependencyNode imagePlaneNode(imagePlaneObj);

    const auto usdImagePlanePath = usdImagePlanePrim.GetPath();
    if (isCompacted) { // we don't know the shape's name, add the shape thingy there
        const auto shapeName = usdImagePlanePrim.GetName().GetString() + std::string("Shape");
        transformNode.setName(usdImagePlanePrim.GetName().GetString().c_str());
        imagePlaneNode.setName(shapeName.c_str());
        if (context) {
            context->RegisterNewMayaNode(usdImagePlanePath.GetString(), transformObj);
            context->RegisterNewMayaNode(usdImagePlanePath.AppendChild(TfToken(shapeName)).GetString(), imagePlaneObj);
        }
    } else {
        transformNode.setName(usdImagePlanePrim.GetParent().GetName().GetString().c_str());
        imagePlaneNode.setName(usdImagePlanePrim.GetName().GetString().c_str());
        if (context) {
            context->RegisterNewMayaNode(usdImagePlanePath.GetParentPath().GetString(), transformObj);
            context->RegisterNewMayaNode(usdImagePlanePath.GetString(), imagePlaneObj);
        }
    }

    dagMod.connect(imagePlaneNode.findPlug("message"), cameraNode.findPlug("imagePlane").elementByLogicalIndex(0));
    status = dagMod.doIt();
    if (!status) { TF_WARN("Error creating transform for image plane!"); return false; }

    const auto earliestTimeCode = UsdTimeCode::EarliestTime();
    auto fitPlug = imagePlaneNode.findPlug("fit");

    TfToken fit;
    usdImagePlane.GetFitAttr().Get(&fit, earliestTimeCode);
    if (fit == MayaImagePlaneWriter::image_plane_best) {
        fitPlug.setShort(MayaImagePlaneWriter::IMAGE_PLANE_FIT_BEST);
    } else if (fit == MayaImagePlaneWriter::image_plane_fill) {
        fitPlug.setShort(MayaImagePlaneWriter::IMAGE_PLANE_FIT_FILL);
    } else if (fit == MayaImagePlaneWriter::image_plane_horizontal) {
        fitPlug.setShort(MayaImagePlaneWriter::IMAGE_PLANE_FIT_HORIZONTAL);
    } else if (fit == MayaImagePlaneWriter::image_plane_vertical) {
        fitPlug.setShort(MayaImagePlaneWriter::IMAGE_PLANE_FIT_VERTICAL);
    } else if (fit == MayaImagePlaneWriter::image_plane_to_size) {
        fitPlug.setShort(MayaImagePlaneWriter::IMAGE_PLANE_FIT_TO_SIZE);
    }

    auto imageNamePlug = imagePlaneNode.findPlug("imageName");
    SdfAssetPath imageAssetPath;
    usdImagePlane.GetFilenameAttr().Get(&imageAssetPath, earliestTimeCode);
    imageNamePlug.setString(imageAssetPath.GetAssetPath().c_str());

    if (!TranslateUsdAttributeToPlug(
        usdImagePlane.GetSizeAttr(), imagePlaneNode, sizeToken, args, context)) {
        return false;
    }

     if (!TranslateUsdAttributeToPlug(
        usdImagePlane.GetOffsetAttr(), imagePlaneNode, offsetToken, args, context)) {
        return false;
    }

    if (!TranslateUsdAttributeToPlug(
        usdImagePlane.GetRotateAttr(), imagePlaneNode, rotateToken, args, context)) {
        return false;
    }

    if (!TranslateUsdAttributeToPlug(
        usdImagePlane.GetCoverageAttr(), imagePlaneNode, coverageToken, args, context)) {
        return false;
    }

    return TranslateUsdAttributeToPlug(
        usdImagePlane.GetCoverageOriginAttr(), imagePlaneNode, coverageOriginToken, args, context);
}
