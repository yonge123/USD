#include "imagePlane.h"

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/imagePlaneShaderKey.h"
#include "pxr/imaging/hdSt/meshTopology.h"

#include "pxr/imaging/hd/vtBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStImagePlane::HdStImagePlane(const SdfPath& id, const SdfPath& instanceId)
    : HdImagePlane(id, instanceId), _topology(), _topologyId(0) {

}

void HdStImagePlane::Sync(
    HdSceneDelegate* delegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits,
    const TfToken& reprName,
    bool forcedRepr) {
    HdRprim::_Sync(delegate, reprName, forcedRepr, dirtyBits);

    auto calcReprName = _GetReprName(delegate, reprName, forcedRepr, dirtyBits);
    _GetRepr(delegate, calcReprName, dirtyBits);
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

HdDirtyBits
HdStImagePlane::_GetInitialDirtyBits() const {
    HdDirtyBits mask = HdChangeTracker::Clean
                       | HdChangeTracker::InitRepr
                       | HdChangeTracker::DirtyPoints
                       | HdChangeTracker::DirtyTopology
                       | HdChangeTracker::DirtyPrimID
                       | HdChangeTracker::DirtyRepr
                       | HdChangeTracker::DirtyMaterialId
                       | HdChangeTracker::DirtyVisibility;

    return mask;
}

HdDirtyBits
HdStImagePlane::_PropagateDirtyBits(HdDirtyBits bits) const {
    return bits;
}

void
HdStImagePlane::_InitRepr(TfToken const &reprName, HdDirtyBits *dirtyBits) {
    const auto& descs = _GetReprDesc(reprName);

    if (_reprs.empty()) {
        _reprs.emplace_back(reprName, boost::make_shared<HdRepr>());

        auto& repr = _reprs.back().second;

        for (const auto& desc: descs) {
            if (desc.geomStyle == HdImagePlaneGeomStyleInvalid) { continue; }
            auto* drawItem = new HdStDrawItem(&_sharedData);
            repr->AddDrawItem(drawItem);
        }

        *dirtyBits |= HdChangeTracker::NewRepr;
    }
}

void
HdStImagePlane::_UpdateDrawItem(
    HdSceneDelegate* sceneDelegate,
    HdStDrawItem* drawItem,
    HdDirtyBits* dirtyBits) {

    _UpdateVisibility(sceneDelegate, dirtyBits);
    // TODO: replace this, since we have quite special needs.
    _PopulateConstantPrimVars(sceneDelegate, drawItem, dirtyBits);
    // TODO: we need to figure out what's required here
    // essentially we don't need any materials here, imagePlane.glslfx should
    // be able to handle everything.
    drawItem->SetMaterialShaderFromRenderIndex(
        sceneDelegate->GetRenderIndex(), GetMaterialId());

    HdSt_ImagePlaneShaderKey shaderKey;
    HdStResourceRegistrySharedPtr resourceRegistry =
         boost::static_pointer_cast<HdStResourceRegistry>(
             sceneDelegate->GetRenderIndex().GetResourceRegistry());
    drawItem->SetGeometricShader(
        HdSt_GeometricShader::Create(shaderKey, resourceRegistry));

    // TODO: We'll need points there and later on uvs
    // to control the texture mapping.
    const auto& id = GetId();
    if (HdChangeTracker::IsAnyPrimVarDirty(*dirtyBits, id)) {
        _PopulateVertexPrimVars(id, sceneDelegate, drawItem, dirtyBits);
    }

    if (*dirtyBits & HdChangeTracker::DirtyTopology) {
        _PopulateTopology(id, sceneDelegate, drawItem, dirtyBits);
    }

    // VertexPrimVar may be null, if there are no points in the prim.
    TF_VERIFY(drawItem->GetConstantPrimVarRange());
}

void
HdStImagePlane::_PopulateVertexPrimVars(
    const SdfPath& id,
    HdSceneDelegate* sceneDelegate,
    HdStDrawItem* drawItem,
    HdDirtyBits* dirtyBits) {

    const HdStResourceRegistrySharedPtr& resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());

    TfTokenVector primVarNames = GetPrimVarVertexNames(sceneDelegate);
    const TfTokenVector& vars = GetPrimVarVaryingNames(sceneDelegate);
    primVarNames.insert(primVarNames.end(), vars.begin(), vars.end());

    HdBufferSourceVector sources;
    sources.reserve(primVarNames.size());

    size_t pointsIndexInSourceArray = std::numeric_limits<size_t>::max();

    for (const auto& nameIt: primVarNames) {
        if (!HdChangeTracker::IsPrimVarDirty(*dirtyBits, id, nameIt)) {
            continue;
        }

        auto value = GetPrimVar(sceneDelegate, nameIt);

        if (!value.IsEmpty()) {
            if (nameIt == HdTokens->points) {
                pointsIndexInSourceArray = sources.size();
            }

            HdBufferSourceSharedPtr source(new HdVtBufferSource(nameIt, value));
            sources.push_back(source);
        }
    }

    if (sources.empty()) { return; }

    auto vertexPrimVarRange = drawItem->GetVertexPrimVarRange();
    if (!vertexPrimVarRange ||
        !vertexPrimVarRange->IsValid()) {
        HdBufferSpecVector bufferSpecs;
        for (auto& it: sources) {
            it->AddBufferSpecs(&bufferSpecs);
        }

        auto range = resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->primVar, bufferSpecs);
        _sharedData.barContainer.Set(
            drawItem->GetDrawingCoord()->GetVertexPrimVarIndex(), range);
    } else if (pointsIndexInSourceArray != std::numeric_limits<size_t>::max()) {
        const auto previousRange = drawItem->GetVertexPrimVarRange()->GetNumElements();
        const auto newRange = sources[pointsIndexInSourceArray]->GetNumElements();

        if (previousRange != newRange) {
            sceneDelegate->GetRenderIndex().GetChangeTracker().SetGarbageCollectionNeeded();
        }
    }

    resourceRegistry->AddSources(
        drawItem->GetVertexPrimVarRange(), sources);
}

void
HdStImagePlane::_PopulateTopology(
    const SdfPath& id,
    HdSceneDelegate* sceneDelegate,
    HdStDrawItem* drawItem,
    HdDirtyBits* dirtyBits) {

    const HdStResourceRegistrySharedPtr& resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());

    if (HdChangeTracker::IsTopologyDirty(*dirtyBits, id)) {
        auto meshTopology = sceneDelegate->GetMeshTopology(id);
        HdSt_MeshTopologySharedPtr topology = HdSt_MeshTopology::New(meshTopology, 0);

        _topologyId = topology->ComputeHash();

        {
            HdInstance<HdTopology::ID, HdMeshTopologySharedPtr> topologyInstance;
            std::unique_lock<std::mutex> regLock =
                resourceRegistry->RegisterMeshTopology(_topologyId, &topologyInstance);

            if (topologyInstance.IsFirstInstance()) {
                topologyInstance.SetValue(
                    boost::static_pointer_cast<HdMeshTopology>(topology));
            }

            _topology = boost::static_pointer_cast<HdSt_MeshTopology>(topologyInstance.GetValue());
        }

        TF_VERIFY(_topology);
    }

    {
        HdInstance<HdTopology::ID, HdBufferArrayRangeSharedPtr> rangeInstance;

        std::unique_lock<std::mutex> regLock =
            resourceRegistry->RegisterMeshIndexRange(_topologyId, HdTokens->indices, &rangeInstance);

        if (rangeInstance.IsFirstInstance()) {
            HdBufferSourceSharedPtr source = _topology->GetTriangleIndexBuilderComputation(GetId());

            HdBufferSourceVector sources;
            sources.push_back(source);

            HdBufferSpecVector bufferSpecs;
            HdBufferSpec::AddBufferSpecs(&bufferSpecs, sources);

            HdBufferArrayRangeSharedPtr range =
                resourceRegistry->AllocateNonUniformBufferArrayRange(
                    HdTokens->topology, bufferSpecs);

            resourceRegistry->AddSources(range, sources);
            rangeInstance.SetValue(range);
            if (drawItem->GetTopologyRange()) {
                sceneDelegate->GetRenderIndex().GetChangeTracker().SetGarbageCollectionNeeded();
            }
        }

        _sharedData.barContainer.Set(drawItem->GetDrawingCoord()->GetTopologyIndex(),
                                     rangeInstance.GetValue());
    }
}

const HdReprSharedPtr&
HdStImagePlane::_GetRepr(
    HdSceneDelegate* sceneDelegate,
    const TfToken& reprName,
    HdDirtyBits* dirtyBits) {
    //auto descs = _GetReprDesc(reprName);
    if (_reprs.empty()) {
        TF_CODING_ERROR("_InitRepr() should be called for repr %s.",
                        reprName.GetText());
        static const HdReprSharedPtr ERROR_RETURN;
        return ERROR_RETURN;
    }
    auto it = _reprs.begin();

    if (HdChangeTracker::IsDirty(*dirtyBits)) {
        auto* drawItem = static_cast<HdStDrawItem*>(_reprs[0].second->GetDrawItem(0));
        _UpdateDrawItem(
            sceneDelegate,
            drawItem,
            dirtyBits);
        *dirtyBits &= ~HdChangeTracker::NewRepr;
    }
    return it->second;
}

PXR_NAMESPACE_CLOSE_SCOPE

