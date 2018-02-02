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
#include "pxr/usd/usdGeom/imagePlane.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomImagePlane,
        TfType::Bases< UsdGeomBoundable > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ImagePlane")
    // to find TfType<UsdGeomImagePlane>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomImagePlane>("ImagePlane");
}

/* virtual */
UsdGeomImagePlane::~UsdGeomImagePlane()
{
}

/* static */
UsdGeomImagePlane
UsdGeomImagePlane::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomImagePlane();
    }
    return UsdGeomImagePlane(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomImagePlane
UsdGeomImagePlane::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ImagePlane");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomImagePlane();
    }
    return UsdGeomImagePlane(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdGeomImagePlane::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomImagePlane>();
    return tfType;
}

/* static */
bool 
UsdGeomImagePlane::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomImagePlane::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomImagePlane::GetFilenameAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->infoFilename);
}

UsdAttribute
UsdGeomImagePlane::CreateFilenameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->infoFilename,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetFrameAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->frame);
}

UsdAttribute
UsdGeomImagePlane::CreateFrameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->frame,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetFitAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->fit);
}

UsdAttribute
UsdGeomImagePlane::CreateFitAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->fit,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetOffsetAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->offset);
}

UsdAttribute
UsdGeomImagePlane::CreateOffsetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->offset,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetSizeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->size);
}

UsdAttribute
UsdGeomImagePlane::CreateSizeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->size,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetRotateAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->rotate);
}

UsdAttribute
UsdGeomImagePlane::CreateRotateAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->rotate,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetCoverageAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->coverage);
}

UsdAttribute
UsdGeomImagePlane::CreateCoverageAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->coverage,
                       SdfValueTypeNames->Int2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomImagePlane::GetCoverageOriginAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->coverageOrigin);
}

UsdAttribute
UsdGeomImagePlane::CreateCoverageOriginAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->coverageOrigin,
                       SdfValueTypeNames->Int2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdGeomImagePlane::GetCameraRel() const
{
    return GetPrim().GetRelationship(UsdGeomTokens->camera);
}

UsdRelationship
UsdGeomImagePlane::CreateCameraRel() const
{
    return GetPrim().CreateRelationship(UsdGeomTokens->camera,
                       /* custom = */ false);
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
UsdGeomImagePlane::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->infoFilename,
        UsdGeomTokens->frame,
        UsdGeomTokens->fit,
        UsdGeomTokens->offset,
        UsdGeomTokens->size,
        UsdGeomTokens->rotate,
        UsdGeomTokens->coverage,
        UsdGeomTokens->coverageOrigin,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomBoundable::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
