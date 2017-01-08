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
#include "pxr/usd/usdHydra/spotLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdHydraSpotLight,
        TfType::Bases< UsdHydraSphereLight > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("SpotLight")
    // to find TfType<UsdHydraSpotLight>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdHydraSpotLight>("SpotLight");
}

/* virtual */
UsdHydraSpotLight::~UsdHydraSpotLight()
{
}

/* static */
UsdHydraSpotLight
UsdHydraSpotLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdHydraSpotLight();
    }
    return UsdHydraSpotLight(stage->GetPrimAtPath(path));
}

/* static */
UsdHydraSpotLight
UsdHydraSpotLight::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("SpotLight");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdHydraSpotLight();
    }
    return UsdHydraSpotLight(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdHydraSpotLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdHydraSpotLight>();
    return tfType;
}

/* static */
bool 
UsdHydraSpotLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdHydraSpotLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdHydraSpotLight::GetDirectionAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->direction);
}

UsdAttribute
UsdHydraSpotLight::CreateDirectionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->direction,
                       SdfValueTypeNames->Float3,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraSpotLight::GetConeAngleAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->coneAngle);
}

UsdAttribute
UsdHydraSpotLight::CreateConeAngleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->coneAngle,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraSpotLight::GetPenumbraAngleAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->penumbraAngle);
}

UsdAttribute
UsdHydraSpotLight::CreatePenumbraAngleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->penumbraAngle,
                       SdfValueTypeNames->Float,
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
UsdHydraSpotLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdHydraTokens->direction,
        UsdHydraTokens->coneAngle,
        UsdHydraTokens->penumbraAngle,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdHydraSphereLight::GetSchemaAttributeNames(true),
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
