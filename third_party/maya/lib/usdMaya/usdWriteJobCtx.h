#ifndef PXRUSDMAYA_USDWRITEJOBCTX_H
#define PXRUSDMAYA_USDWRITEJOBCTX_H

#include "pxr/pxr.h"
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
    usdWriteJobCtx(const JobExportArgs& args);
    const JobExportArgs& getArgs() const { return mArgs; };
    const UsdStageRefPtr& getUsdStage() const { return mStage; };
    // Querying the master path for instancing. This also creates the mesh if it doesn't exists.
    SdfPath getMasterPath(const MDagPath& dg);
protected:
    bool openFile(const std::string& filename, bool append);
    void saveAndCloseStage();
    MayaPrimWriterPtr createPrimWriter(const MDagPath& curDag);

    JobExportArgs mArgs;
    // List of the primitive writers to iterate over
    std::vector<MayaPrimWriterPtr> mMayaPrimWriterList;
    // Stage used to write out USD file
    UsdStageRefPtr mStage;
private:
    SdfPath getUsdPathFromDagPath(const MDagPath& dagPath, bool instanceSource);

    struct MObjectHandleComp {
        bool operator()(const MObjectHandle& rhs, const MObjectHandle& lhs) const {
            return rhs.hashCode() < lhs.hashCode();
        }
    };
    std::map<MObjectHandle, SdfPath, MObjectHandleComp> mMasterToUsdPath;
    MayaPrimWriterPtr _createPrimWriter(const MDagPath& curDag, bool instanceSource);
    UsdPrim mInstancesScope;
    SdfPath mParentScopePath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
