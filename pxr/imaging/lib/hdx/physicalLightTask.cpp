#include "physicalLightTask.h"

#include "pxr/imaging/hdx/physicalLightingShader.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hdx/light.h"
#include "pxr/base/vt/value.h"

HdxPhysicalLightTask::HdxPhysicalLightTask(HdSceneDelegate* delegate, const SdfPath& id) :
    HdSceneTask(delegate, id),
    _camera(),
    _lights(),
    _lightingShader(new HdxPhysicalLightingShader()),
    _glfPhysicalLights(),
    _viewport(),
    _collectionVersion(0)
{

}

/* virtual */
void
HdxPhysicalLightTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();
}

/* virtual */
void
HdxPhysicalLightTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();

    (*ctx)[HdxTokens->physicalLightingShader] =
        boost::dynamic_pointer_cast<HdLightingShader>(_lightingShader);

    _TaskDirtyState dirtyState;

    // this is used to check if geometry + camera has changed
    // and rebuilding the shadow / GI maps is required
    _GetTaskDirtyState(HdTokens->geometry, &dirtyState);

    const bool collectionChanged = (_collectionVersion != dirtyState.collectionVersion);

    if ((dirtyState.bits & HdChangeTracker::DirtyParams) ||
        collectionChanged) {
        _collectionVersion = dirtyState.collectionVersion;

        HdxPhysicalLightTaskParams params;
        if (not _GetSceneDelegateValue(HdTokens->params, &params)) {
            return;
        }

        HdSceneDelegate* delegate = GetDelegate();
        _camera = delegate->GetRenderIndex().GetSprim(params.cameraPath);
        _viewport = params.viewport;

        auto sprims = delegate->GetRenderIndex().GetSprimSubtree(SdfPath::AbsoluteRootPath());

        _lights.clear();
        _lights.reserve(sprims.size());

        for (const auto& sprim_path : sprims) {
            const auto& sprim = delegate->GetRenderIndex().GetSprim(sprim_path);
            if (boost::dynamic_pointer_cast<HdxLight>(sprim)) {
                _lights.push_back(sprim);
            }
        }
    }

    if (not TF_VERIFY(_camera)) {
        return;
    }

    auto lightingContext = _lightingShader->GetLightingContext();
    (*ctx)[HdxTokens->physicalLightingContext] = lightingContext;

    VtValue modelViewMatrix = _camera->Get(HdShaderTokens->worldToViewMatrix);
    VtValue projectionMatrix = _camera->Get(HdShaderTokens->projectionMatrix);
    TF_VERIFY(modelViewMatrix.IsHolding<GfMatrix4d>());
    TF_VERIFY(projectionMatrix.IsHolding<GfMatrix4d>());

    size_t num_lights = _lights.size();

    _glfPhysicalLights.clear();
    if (num_lights != _glfPhysicalLights.capacity())
    {
        _glfPhysicalLights.reserve(num_lights);
        _glfPhysicalLights.shrink_to_fit();
    }

    for (const auto& light : _lights) {
        VtValue light_params = light->Get(HdxLightTokens->params);

        if (light_params.IsHolding<GlfPhysicalLight>()) {
            _glfPhysicalLights.push_back(light_params.UncheckedGet<GlfPhysicalLight>());
        } else {
            continue;
        }

        auto& glfl = _glfPhysicalLights.back();

        glfl.SetID(light->GetID());
        // we are not doing transformations like for simple lights, because transformations
        // are already done in the light context
    }

    num_lights = _glfPhysicalLights.size();
    lightingContext->SetUseLighting(num_lights > 0);
    lightingContext->SetLights(_glfPhysicalLights);
    lightingContext->SetCamera(modelViewMatrix.Get<GfMatrix4d>(),
                               projectionMatrix.Get<GfMatrix4d>());
}

std::ostream& operator<<(std::ostream& out, const HdxPhysicalLightTaskParams& pv)
{
    out << pv.cameraPath << " " << pv.enableShadows << " " << pv.viewport;
    return out;
}

bool operator==(const HdxPhysicalLightTaskParams& lhs, const HdxPhysicalLightTaskParams& rhs)
{
    return lhs.cameraPath == rhs.cameraPath and
           lhs.enableShadows == rhs.enableShadows and
           lhs.viewport == rhs.viewport;
}
bool operator!=(const HdxPhysicalLightTaskParams& lhs, const HdxPhysicalLightTaskParams& rhs)
{
    return not (lhs == rhs);
}
