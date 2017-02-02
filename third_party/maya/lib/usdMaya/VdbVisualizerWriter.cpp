#include "usdMaya/VdbVisualizerWriter.h"
#include "pxr/usd/usdAi/aiVolume.h"
#include "pxr/usd/sdf/types.h"

#include <maya/MDataHandle.h>

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    const TfToken filename_token("filename");
    const TfToken velocity_grids_token("velocityGrids");
    const TfToken velocity_scale_token("velocityScale");
    const TfToken velocity_fps_token("velocityFps");
    const TfToken velocity_shutter_start_token("velocityShutterStart");
    const TfToken velocity_shutter_end_token("velocityShutterEnd");
    const TfToken bounds_slack_token("boundsSlack");

    UsdAttribute get_attribute(UsdPrim& prim, const TfToken& attr_name, const SdfValueTypeName& type, SdfVariability variability) {
        if (prim.HasAttribute(attr_name)) {
            return prim.GetAttribute(attr_name);
        } else {
            return prim.CreateAttribute(attr_name, type, variability);
        }
    }

    bool export_grids(UsdPrim& prim, MFnDependencyNode& node, const char* maya_attr_name, const TfToken& usd_attr_name) {
        const auto grids_string = node.findPlug(maya_attr_name).asString();
        MStringArray grids;
        grids_string.split(' ', grids);
        const auto grids_length = grids.length();
        if (grids_length > 0) {
            VtStringArray grid_names;
            grid_names.reserve(grids_length);
            for (std::remove_const<decltype(grids_length)>::type i = 0; i < grids_length; ++i) {
                grid_names.push_back(grids[i].asChar());
            }
            get_attribute(prim, usd_attr_name, SdfValueTypeNames.Get()->StringArray, SdfVariabilityUniform)
                .Set(grid_names);
            return true;
        } else {
            return false;
        }
    }
}

VdbVisualizerWriter::VdbVisualizerWriter(MDagPath& iDag, UsdStageRefPtr stage, const JobExportArgs& iArgs) :
    MayaTransformWriter(iDag, stage, iArgs, false), has_velocity_grids(false) {
    UsdAiVolume primSchema =
        UsdAiVolume::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    mUsdPrim = primSchema.GetPrim();
    TF_AXIOM(mUsdPrim);
}

void VdbVisualizerWriter::write(const UsdTimeCode& usdTime) {
    UsdAiVolume primSchema(mUsdPrim);
    writeTransformAttrs(usdTime, primSchema);

    MFnDependencyNode volume_node(getDagPath().node());

    // some of the attributes that don't need to be animated has to be exported here
    if (usdTime.IsDefault()) {
        has_velocity_grids = export_grids(mUsdPrim, volume_node, "velocity_grids", velocity_grids_token);
    }

    if (usdTime.IsDefault() == isShapeAnimated()) {
        return;
    }

    primSchema.GetDsoAttr().Set(std::string("volume_openvdb"));
    const auto out_vdb_path = volume_node.findPlug("outVdbPath").asString();
    const auto& bbox_min = volume_node.findPlug("bboxMin").asMDataHandle().asFloat3();
    const auto& bbox_max = volume_node.findPlug("bboxMax").asMDataHandle().asFloat3();
    VtVec3fArray extents(2);
    extents[0] = GfVec3f(bbox_min[0], bbox_min[1], bbox_min[2]);
    extents[1] = GfVec3f(bbox_max[0], bbox_max[1], bbox_max[2]);
    primSchema.GetExtentAttr().Set(extents, usdTime);

    const auto sampling_quality = volume_node.findPlug("samplingQuality").asFloat();
    primSchema.GetStepSizeAttr().Set(volume_node.findPlug("voxelSize").asFloat() / (sampling_quality / 100.0f), usdTime);
    primSchema.GetMatteAttr().Set(volume_node.findPlug("matte").asBool(), usdTime);
    primSchema.GetReceiveShadowsAttr().Set(volume_node.findPlug("receiveShadows").asBool(), usdTime);
    primSchema.GetSelfShadowsAttr().Set(volume_node.findPlug("selfShadows").asBool(), usdTime);
    get_attribute(mUsdPrim, filename_token, SdfValueTypeNames.Get()->String, SdfVariabilityUniform)
        .Set(std::string(out_vdb_path.asChar()), usdTime);

    if (has_velocity_grids) {
        get_attribute(mUsdPrim, velocity_scale_token, SdfValueTypeNames.Get()->Float, SdfVariabilityUniform)
            .Set(volume_node.findPlug("velocityScale").asFloat(), usdTime);
        get_attribute(mUsdPrim, velocity_fps_token, SdfValueTypeNames.Get()->Float, SdfVariabilityUniform)
            .Set(volume_node.findPlug("velocityFps").asFloat(), usdTime);
        get_attribute(mUsdPrim, velocity_shutter_start_token, SdfValueTypeNames.Get()->Float, SdfVariabilityUniform)
            .Set(volume_node.findPlug("velocityShutterStart").asFloat(), usdTime);
        get_attribute(mUsdPrim, velocity_shutter_end_token, SdfValueTypeNames.Get()->Float, SdfVariabilityUniform)
            .Set(volume_node.findPlug("velocityShutterEnd").asFloat(), usdTime);
    }

    get_attribute(mUsdPrim, bounds_slack_token, SdfValueTypeNames.Get()->Float, SdfVariabilityUniform)
        .Set(volume_node.findPlug("boundsSlack").asFloat());
}
