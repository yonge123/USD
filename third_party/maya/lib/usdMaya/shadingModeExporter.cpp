#include "usdMaya/shadingModeExporter.h"

#include <maya/MItDependencyNodes.h>

PXR_NAMESPACE_OPEN_SCOPE

PxrUsdMayaShadingModeExporter::PxrUsdMayaShadingModeExporter() {

}

PxrUsdMayaShadingModeExporter::~PxrUsdMayaShadingModeExporter() {

}

void
PxrUsdMayaShadingModeExporter::DoExport(
    const UsdStageRefPtr& stage,
    const PxrUsdMayaUtil::ShapeSet& bindableRoots,
    bool mergeTransformAndShape,
    bool handleUsdNamespaces,
    const SdfPath& overrideRootPath) {
    MItDependencyNodes shadingEngineIter(MFn::kShadingEngine);
    for (; !shadingEngineIter.isDone(); shadingEngineIter.next()) {
        MObject shadingEngine(shadingEngineIter.thisNode());

        PxrUsdMayaShadingModeExportContext c(
            shadingEngine,
            stage,
            mergeTransformAndShape,
            handleUsdNamespaces,
            bindableRoots,
            overrideRootPath);

        Export(c);
    }
}

void
PxrUsdMayaShadingModeExporter::Export(const PxrUsdMayaShadingModeExportContext& ctx) {

}

PXR_NAMESPACE_CLOSE_SCOPE
