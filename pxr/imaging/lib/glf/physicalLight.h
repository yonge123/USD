#pragma once

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/usd/sdf/path.h"
#include <ostream>

enum PhysicalLightTypes {
    PHYSICAL_LIGHT_DISTANT = 0,
    PHYSICAL_LIGHT_SPHERE,
    PHYSICAL_LIGHT_SPOT,
    PHYSICAL_LIGHT_QUAD,
    PHSYICAL_LIGHT_SKY
};

class GlfPhysicalLight {
public:
    GlfPhysicalLight(PhysicalLightTypes lightType = PHYSICAL_LIGHT_DISTANT);
    virtual ~GlfPhysicalLight();

    const GfMatrix4d& GetTransform() const;
    void SetTransform(const GfMatrix4d& mat);

    const GfVec3f& GetColor() const;
    void SetColor(const GfVec3f& color);

    const GfVec3f& GetDirection() const;
    void SetDirection(const GfVec3f& direction);

    const GfVec2f& GetAttenuation() const;
    void SetAttenuation(const GfVec2f& attenuation);

    PhysicalLightTypes GetLightType() const;
    void SetLightType(PhysicalLightTypes lightType);

    float GetDiffuse() const;
    void SetDiffuse(float diffuse);

    float GetSpecular() const;
    void SetSpecular(float specular);

    float GetIndirect() const;
    void SetIndirect(float indirect);

    float GetSpread() const;
    void SetSpread(float spread);

    float GetRadius() const;
    void SetRadius(float radius);

    float GetConeAngle() const;
    void SetConeAngle(float coneAngle);

    float GetPenumbraAngle() const;
    void SetPenumbraAngle(float penumbraAngle);

    bool GetHasShadow() const;
    void SetHasShadow(bool hasShadow);

    const SdfPath& GetID() const;
    void SetID(const SdfPath& id);

    bool operator ==(const GlfPhysicalLight& other) const;
    bool operator !=(const GlfPhysicalLight& other) const;
private:
    friend std::ostream & operator <<(std::ostream& out, const GlfPhysicalLight& v);

    SdfPath _id;

    GfMatrix4d _transform;

    GfVec3f _color;
    GfVec3f _direction;
    GfVec2f _attenuation;

    PhysicalLightTypes _lightType;

    float _intensity;
    float _specular;
    float _diffuse;
    float _indirect;
    float _spread;
    float _radius;
    float _coneAngle;
    float _penumbraAngle;

    bool _hasShadow;
};

std::ostream& operator<<(std::ostream& out, const GlfPhysicalLight& v);

typedef std::vector<class GlfPhysicalLight> GlfPhysicalLightVector;

// VtValue requirements
std::ostream& operator<<(std::ostream& out, const GlfPhysicalLightVector& pv);