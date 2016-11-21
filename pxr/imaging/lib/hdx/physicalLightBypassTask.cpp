#include "physicalLightBypassTask.h"

#include "pxr/imaging/hdx/physicalLightingShader.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"

HdxPhysicalLightBypassTask::HdxPhysicalLightBypassTask(HdSceneDelegate* delegate,
                                                       SdfPath const& id)
    : HdSceneTask(delegate, id)
    , _lightingShader(new HdxPhysicalLightingShader())
    , _physicalLightingContext()
{
}

void
HdxPhysicalLightBypassTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();
}

void
HdxPhysicalLightBypassTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();

    HdChangeTracker::DirtyBits bits = _GetTaskDirtyBits();

    if (bits & HdChangeTracker::DirtyParams) {
        HdxPhysicalLightBypassTaskParams params;
        if (not _GetSceneDelegateValue(HdTokens->params, &params)) {
            return;
        }

        _physicalLightingContext = params.physicalLightingContext;
        _camera = GetDelegate()->GetRenderIndex().GetSprim(params.cameraPath);
    }

    if (_physicalLightingContext) {
        if (not TF_VERIFY(_camera)) {
            return;
        }

        VtValue modelViewMatrix = _camera->Get(HdShaderTokens->worldToViewMatrix);
        if (not TF_VERIFY(modelViewMatrix.IsHolding<GfMatrix4d>())) return;
        VtValue projectionMatrix = _camera->Get(HdShaderTokens->projectionMatrix);
        if (not TF_VERIFY(projectionMatrix.IsHolding<GfMatrix4d>())) return;

        // need camera matrices to compute lighting paramters in the eye-space.
        //
        // you should be a bit careful here...
        //
        // _simpleLightingContext->SetCamera() is useless, since
        // _lightingShader->SetLightingState() actually only copies the lighting
        // paramters, not the camera matrices.
        // _lightingShader->SetCamera() is the right one.
        //
        _lightingShader->SetLightingState(_physicalLightingContext);
        _lightingShader->SetCamera(modelViewMatrix.Get<GfMatrix4d>(),
                                   projectionMatrix.Get<GfMatrix4d>());
    }

    // Done at end, because the lighting context can be changed above.
    // Also we want the context in the shader as it's only a partial copy
    // of the context we own.
    (*ctx)[HdxTokens->physicalLightingShader]  = boost::dynamic_pointer_cast<HdLightingShader>(_lightingShader);
    (*ctx)[HdxTokens->physicalLightingContext] = _lightingShader->GetLightingContext();

}

std::ostream& operator<<(std::ostream& out,
                         const HdxPhysicalLightBypassTaskParams& pv)
{
    return out;
}

bool operator==(const HdxPhysicalLightBypassTaskParams& lhs,
                const HdxPhysicalLightBypassTaskParams& rhs) {
    return lhs.cameraPath == rhs.cameraPath
           and lhs.physicalLightingContext == rhs.physicalLightingContext;
}

bool operator!=(const HdxPhysicalLightBypassTaskParams& lhs,
                const HdxPhysicalLightBypassTaskParams& rhs) {
    return not(lhs == rhs);
}
