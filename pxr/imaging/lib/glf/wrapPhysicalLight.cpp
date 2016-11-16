#include "pxr/imaging/glf/physicalLight.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>

using namespace boost::python;

void wrapPhysicalLight()
{
    typedef GlfPhysicalLight This;

    enum_<PhysicalLightTypes>("PhysicalLightTypes")
        .value("distant", PHYSICAL_LIGHT_DISTANT)
        .value("sphere", PHYSICAL_LIGHT_SPHERE)
        .value("spot", PHYSICAL_LIGHT_SPOT)
        .value("quad", PHYSICAL_LIGHT_QUAD)
        .value("sky", PHYSICAL_LIGHT_SKY);

    class_<This>("PhysicalLight", init<>())
        .add_property("transform",
                      make_function(
                          &This::GetTransform,
                          return_value_policy<return_by_value>()),
                      &This::SetTransform)
        .add_property("color",
                      make_function(
                          &This::GetColor,
                          return_value_policy<return_by_value>()),
                      &This::SetColor)
        .add_property("direction",
                      make_function(
                          &This::GetDirection,
                          return_value_policy<return_by_value>()),
                      &This::SetDirection)
        .add_property("attenuation",
                      make_function(
                          &This::GetAttenuation,
                          return_value_policy<return_by_value>()),
                      &This::SetAttenuation)
        .add_property("lightType",
                      make_function(
                          &This::GetLightType,
                          return_value_policy<return_by_value>()),
                      &This::SetLightType)
        .add_property("diffuse",
                      make_function(
                          &This::GetDiffuse,
                          return_value_policy<return_by_value>()),
                      &This::SetDiffuse)
        .add_property("specular",
                      make_function(
                          &This::GetSpecular,
                          return_value_policy<return_by_value>()),
                      &This::SetSpecular)
        .add_property("indirect",
                      make_function(
                          &This::GetIndirect,
                          return_value_policy<return_by_value>()),
                      &This::SetIndirect)
        .add_property("spread",
                      make_function(
                          &This::GetSpread,
                          return_value_policy<return_by_value>()),
                      &This::SetSpread)
        .add_property("radius",
                      make_function(
                          &This::GetRadius,
                          return_value_policy<return_by_value>()),
                      &This::SetRadius)
        .add_property("coneAngle",
                      make_function(
                          &This::GetConeAngle,
                          return_value_policy<return_by_value>()),
                      &This::SetConeAngle)
        .add_property("penumbraAngle",
                      make_function(
                          &This::GetPenumbraAngle,
                          return_value_policy<return_by_value>()),
                      &This::SetPenumbraAngle)
        .add_property("hasShadow",
                      make_function(
                          &This::GetHasShadow,
                          return_value_policy<return_by_value>()),
                      &This::SetHasShadow)
        .add_property("id",
                      make_function(
                          &This::GetID,
                          return_value_policy<return_by_value>()),
                      &This::SetID);
}
