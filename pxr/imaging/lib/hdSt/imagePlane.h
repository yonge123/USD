#ifndef HDST_IMAGEPLANE_H
#define HDST_IMAGEPLANE_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/imaging/hd/imagePlane.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hdSt/drawItem.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdSt_MeshTopology> HdSt_MeshTopologySharedPtr;

class HdStImagePlane final : public HdImagePlane {
public:
    HdStImagePlane(const SdfPath& id,
                   const SdfPath& instanceId);

    void Sync(
        HdSceneDelegate* delegate,
        HdRenderParam* renderParam,
        HdDirtyBits* dirtyBits,
        const TfToken& reprName,
        bool forcedRepr) override;

    HdDirtyBits _GetInitialDirtyBits() const override;
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    void _InitRepr(TfToken const &reprName, HdDirtyBits *dirtyBits) override;

    const HdReprSharedPtr&
    _GetRepr(
        HdSceneDelegate* sceneDelegate,
        const TfToken& reprName,
        HdDirtyBits* dirtyBits) override;

private:
    void
    _UpdateDrawItem(
        HdSceneDelegate* sceneDelegate,
        HdStDrawItem* drawItem,
        HdDirtyBits* dirtyBits);

    void
    _PopulateVertexPrimVars(
        const SdfPath& id,
        HdSceneDelegate *sceneDelegate,
        HdStDrawItem *drawItem,
        HdDirtyBits *dirtyBits);

    void
    _PopulateTopology(
        const SdfPath& id,
        HdSceneDelegate* sceneDelegate,
        HdStDrawItem* drawItem,
        HdDirtyBits* dirtyBits);

    HdSt_MeshTopologySharedPtr _topology;
    HdTopology::ID _topologyId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
