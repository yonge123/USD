#include "physicalLightTask.h"

HdxPhysicalLightTask::HdxPhysicalLightTask(HdSceneDelegate* delegate, const SdfPath& id) :
    HdSceneTask(delegate, id)
{

}

/* virtual */
void
HdxPhysicalLightTask::_Sync(HdTaskContext* ctx)
{

}

/* virtual */
void
HdxPhysicalLightTask::_Execute(HdTaskContext* ctx)
{
}

std::ostream& operator<<(std::ostream& out, const HdxPhysicalLightTaskParams& pv)
{
    out << pv.cameraPath << " " << pv.enableShadows;
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
