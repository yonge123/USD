#ifndef HDX_PHYSICAL_LIGHT_TASK_H
#define HDX_PHYSICAL_LIGHT_TASK_H

#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/glf/physicalLight.h"
#include "pxr/base/gf/vec4f.h"

typedef boost::shared_ptr<class HdxPhysicalLightingShader> HdxPhysicalLightingShaderSharedPtr;

class HdxPhysicalLightTask : public HdSceneTask {
public:
    HdxPhysicalLightTask(HdSceneDelegate* delegate, const SdfPath& id);

protected:
    virtual void _Sync( HdTaskContext* ctx);
    virtual void _Execute(HdTaskContext* ctx);

private:
    HdSprimSharedPtr _camera;
    HdSprimSharedPtrVector _lights;
    HdxPhysicalLightingShaderSharedPtr _lightingShader;
    GlfPhysicalLightVector _glfPhysicalLights;
    GfVec4f _viewport;
    int _collectionVersion;
};

struct HdxPhysicalLightTaskParams {
    HdxPhysicalLightTaskParams () :
        cameraPath(),
        viewport(0.0f),
        enableShadows(false)
    { }

    SdfPath cameraPath;
    GfVec4f viewport;
    bool enableShadows;
};

std::ostream& operator<<(std::ostream& out, const HdxPhysicalLightTaskParams& pv);
bool operator==(const HdxPhysicalLightTaskParams& lhs, const HdxPhysicalLightTaskParams& rhs);
bool operator!=(const HdxPhysicalLightTaskParams& lhs, const HdxPhysicalLightTaskParams& rhs);

#endif
