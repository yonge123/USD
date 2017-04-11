#ifndef PXRUSDMAYA_USDWRITEJOBCTX_H
#define PXRUSDMAYA_USDWRITEJOBCTX_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "pxr/usd/sdf/path.h"
#include "usdMaya/JobArgs.h"

#include <maya/MDagPath.h>
#include <maya/MObjectHandle.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class MayaPrimWriter;
typedef std::shared_ptr<MayaPrimWriter> MayaPrimWriterPtr;

class usdWriteJobCtx {
public:
    PXRUSDMAYA_API
    usdWriteJobCtx(const JobExportArgs& args);
    const JobExportArgs& getArgs() const { return mArgs; };
    const UsdStageRefPtr& getUsdStage() const { return mStage; };
    // Querying the master path for instancing. This also creates the mesh if it doesn't exists.
    SdfPath getMasterPath(const MDagPath& dg);
protected:
    PXRUSDMAYA_API
    bool openFile(const std::string& filename, bool append);
    PXRUSDMAYA_API
    void processInstances();
    PXRUSDMAYA_API
    MayaPrimWriterPtr createPrimWriter(const MDagPath& curDag);

    JobExportArgs mArgs;
    // List of the primitive writers to iterate over
    std::vector<MayaPrimWriterPtr> mMayaPrimWriterList;
    // Stage used to write out USD file
    UsdStageRefPtr mStage;
private:
    PXRUSDMAYA_API
    SdfPath getUsdPathFromDagPath(const MDagPath& dagPath, bool instanceSource);

    struct MObjectHandleComp {
        bool operator()(const MObjectHandle& rhs, const MObjectHandle& lhs) const {
            return rhs.hashCode() < lhs.hashCode();
        }
    };
    std::map<MObjectHandle, SdfPath, MObjectHandleComp> mMasterToUsdPath;
    PXRUSDMAYA_API
    MayaPrimWriterPtr _createPrimWriter(const MDagPath& curDag, bool instanceSource);
    UsdPrim mInstancesPrim;
    SdfPath mParentScopePath;
    bool mNoInstances;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
