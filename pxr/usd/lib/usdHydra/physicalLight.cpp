//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usdHydra/physicalLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdHydraPhysicalLight,
        TfType::Bases< UsdGeomXformable > >();
    
}

/* virtual */
UsdHydraPhysicalLight::~UsdHydraPhysicalLight()
{
}

/* static */
UsdHydraPhysicalLight
UsdHydraPhysicalLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (not stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdHydraPhysicalLight();
    }
    return UsdHydraPhysicalLight(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdHydraPhysicalLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdHydraPhysicalLight>();
    return tfType;
}

/* static */
bool 
UsdHydraPhysicalLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdHydraPhysicalLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdHydraPhysicalLight::GetColorAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->color);
}

UsdAttribute
UsdHydraPhysicalLight::CreateColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->color,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraPhysicalLight::GetIntensityAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->intensity);
}

UsdAttribute
UsdHydraPhysicalLight::CreateIntensityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->intensity,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraPhysicalLight::GetSpecularAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->specular);
}

UsdAttribute
UsdHydraPhysicalLight::CreateSpecularAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->specular,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraPhysicalLight::GetDiffuseAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->diffuse);
}

UsdAttribute
UsdHydraPhysicalLight::CreateDiffuseAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->diffuse,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraPhysicalLight::GetIndirectAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->indirect);
}

UsdAttribute
UsdHydraPhysicalLight::CreateIndirectAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->indirect,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraPhysicalLight::GetAttenuationAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->attenuation);
}

UsdAttribute
UsdHydraPhysicalLight::CreateAttenuationAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->attenuation,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraPhysicalLight::GetHasShadowsAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->hasShadows);
}

UsdAttribute
UsdHydraPhysicalLight::CreateHasShadowsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->hasShadows,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdHydraPhysicalLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdHydraTokens->color,
        UsdHydraTokens->intensity,
        UsdHydraTokens->specular,
        UsdHydraTokens->diffuse,
        UsdHydraTokens->indirect,
        UsdHydraTokens->attenuation,
        UsdHydraTokens->hasShadows,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomXformable::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
