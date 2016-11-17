#include "pxr/imaging/glf/physicalLight.h"

#include <algorithm>

GlfPhysicalLight::GlfPhysicalLight(PhysicalLightTypes lightType) :
    _color(1.0f, 1.0f, 1.0f),
    _direction(-1.0f, 0.0f, 0.0f),
    _attenuation(0.0f, 1.0f),
    _lightType(lightType),
    _intensity(1.0f),
    _specular(1.0f),
    _diffuse(1.0f),
    _indirect(1.0f),
    _spread(0.0f),
    _radius(1.0f),
    _coneAngle(65.0f),
    _penumbraAngle(0.0f),
    _hasShadow(false)
{

}

GlfPhysicalLight::~GlfPhysicalLight()
{

}

const GfMatrix4d& GlfPhysicalLight::GetTransform() const
{
    return _transform;
}

void GlfPhysicalLight::SetTransform(const GfMatrix4d& mat)
{
    _transform = mat;
}

const GfVec3f& GlfPhysicalLight::GetColor() const
{
    return _color;
}

void GlfPhysicalLight::SetColor(const GfVec3f& color)
{
    _color[0] = std::max(0.0f, color[0]);
    _color[1] = std::max(0.0f, color[1]);
    _color[2] = std::max(0.0f, color[2]);
}

const GfVec3f& GlfPhysicalLight::GetDirection() const
{
    return _direction;
}

void GlfPhysicalLight::SetDirection(const GfVec3f& direction)
{
    _direction = direction;
    const float l = _direction.GetLength();
    if (l < GF_MIN_VECTOR_LENGTH)
        _direction = GfVec3f(-1.0f, 0.0f, 0.0f);
    else
        _direction *= 1.0f / l;
}

const GfVec2f& GlfPhysicalLight::GetAttenuation() const
{
    return _attenuation;
}

void GlfPhysicalLight::SetAttenuation(const GfVec2f& attenuation)
{
    _attenuation[0] = std::max(0.0f, attenuation[0]);
    _attenuation[1] = std::max(0.0f, attenuation[1]);
}

PhysicalLightTypes GlfPhysicalLight::GetLightType() const
{
    return _lightType;
}

void GlfPhysicalLight::SetLightType(PhysicalLightTypes lightType)
{
    _lightType = lightType;
}

float GlfPhysicalLight::GetIntensity() const
{
    return _intensity;
}

void GlfPhysicalLight::SetIntensity(float intensity)
{
    _intensity = std::max(0.0f, intensity);
}

float GlfPhysicalLight::GetDiffuse() const
{
    return _diffuse;
}

void GlfPhysicalLight::SetDiffuse(float diffuse)
{
    _diffuse = std::max(0.0f, diffuse);
}

float GlfPhysicalLight::GetSpecular() const
{
    return _specular;
}

void GlfPhysicalLight::SetSpecular(float specular)
{
    _specular = std::max(0.0f, specular);
}

float GlfPhysicalLight::GetIndirect() const
{
    return _indirect;
}

void GlfPhysicalLight::SetIndirect(float indirect)
{
    _indirect = std::max(0.0f, indirect);
}

float GlfPhysicalLight::GetSpread() const
{
    return _spread;
}

void GlfPhysicalLight::SetSpread(float spread)
{
    _spread = std::max(0.0f, spread);
}

float GlfPhysicalLight::GetRadius() const
{
    return _radius;
}

void GlfPhysicalLight::SetRadius(float radius)
{
    _radius = std::max(0.0f, radius);
}

float GlfPhysicalLight::GetConeAngle() const
{
    return _coneAngle;
}

void GlfPhysicalLight::SetConeAngle(float coneAngle)
{
    _coneAngle = std::max(0.0f, coneAngle);
}

float GlfPhysicalLight::GetPenumbraAngle() const
{
    return _penumbraAngle;
}

void GlfPhysicalLight::SetPenumbraAngle(float penumbraAngle)
{
    _penumbraAngle = penumbraAngle;
}

bool GlfPhysicalLight::GetHasShadow() const
{
    return _hasShadow;
}

void GlfPhysicalLight::SetHasShadow(bool hasShadow)
{
    _hasShadow = hasShadow;
}

const SdfPath& GlfPhysicalLight::GetID() const
{
    return _id;
}

void GlfPhysicalLight::SetID(const SdfPath& id)
{
    _id = id;
}

bool GlfPhysicalLight::operator==(const GlfPhysicalLight& other) const
{
    return _id == other._id and
           _transform == other._transform and
           _color == other._color and
           _direction == other._direction and
           _attenuation == other._attenuation and
           _lightType == other._lightType and
           _intensity == other._intensity and
           _specular == other._specular and
           _diffuse == other._diffuse and
           _indirect == other._indirect and
           _spread == other._spread and
           _radius == other._radius and
           _coneAngle == other._coneAngle and
           _penumbraAngle == other._penumbraAngle and
           _hasShadow == other._hasShadow;
}

bool GlfPhysicalLight::operator!=(const GlfPhysicalLight& other) const
{
    return not(*this == other);
}

std::ostream& operator<<(std::ostream& out, const GlfPhysicalLight& v)
{
    out << v._id
        << v._lightType
        << v._transform
        << v._color
        << v._direction
        << v._attenuation
        << v._intensity
        << v._specular
        << v._diffuse
        << v._indirect
        << v._spread
        << v._radius
        << v._coneAngle
        << v._penumbraAngle
        << v._hasShadow;
    return out;
}

std::ostream&
operator<<(std::ostream& out, const GlfPhysicalLightVector& pv)
{
    for (const auto& v : pv)
        out << v;
    return out;
}
