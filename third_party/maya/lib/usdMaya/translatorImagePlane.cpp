#include "translatorImagePlane.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnCamera.h>
#include <maya/MDagModifier.h>
#include <maya/MPlug.h>

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
    if (!status) { std::cerr << "Error creating transform for image plane!" << std::endl; return false; }
    MObject imagePlaneObj = dagMod.createNode("imagePlane", transformObj, &status);
    if (!status) { std::cerr << "Error creating image plane!" << std::endl; return false; }
    status = dagMod.doIt();
    if (!status) { std::cerr << "Error executing dagMod creating nodes!" << std::endl; return false; }

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
    if (!status) { std::cerr << "Error executing dagMod connecting attributes!" << std::endl; return false; }

    return true;
}
