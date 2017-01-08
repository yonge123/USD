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
#ifndef USDHYDRA_GENERATED_PHYSICALLIGHT_H
#define USDHYDRA_GENERATED_PHYSICALLIGHT_H

/// \file usdHydra/physicalLight.h

#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdHydra/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// PHYSICALLIGHT                                                              //
// -------------------------------------------------------------------------- //

/// \class UsdHydraPhysicalLight
///
///
class UsdHydraPhysicalLight : public UsdGeomXformable
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Construct a UsdHydraPhysicalLight on UsdPrim \p prim .
    /// Equivalent to UsdHydraPhysicalLight::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdHydraPhysicalLight(const UsdPrim& prim=UsdPrim())
        : UsdGeomXformable(prim)
    {
    }

    /// Construct a UsdHydraPhysicalLight on the prim held by \p schemaObj .
    /// Should be preferred over UsdHydraPhysicalLight(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdHydraPhysicalLight(const UsdSchemaBase& schemaObj)
        : UsdGeomXformable(schemaObj)
    {
    }

    /// Destructor.
    virtual ~UsdHydraPhysicalLight();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdHydraPhysicalLight holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdHydraPhysicalLight(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    static UsdHydraPhysicalLight
    Get(const UsdStagePtr &stage, const SdfPath &path);


private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    virtual const TfType &_GetTfType() const;

public:
    // --------------------------------------------------------------------- //
    // COLOR 
    // --------------------------------------------------------------------- //
    /// Color of the light.
    ///
    /// \n  C++ Type: GfVec3f
    /// \n  Usd Type: SdfValueTypeNames->Color3f
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (1, 1, 1)
    UsdAttribute GetColorAttr() const;

    /// See GetColorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateColorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INTENSITY 
    // --------------------------------------------------------------------- //
    /// Intensity of the light.
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 1.0
    UsdAttribute GetIntensityAttr() const;

    /// See GetIntensityAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateIntensityAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // SPECULAR 
    // --------------------------------------------------------------------- //
    /// Weight of the specular contribution.
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 1.0
    UsdAttribute GetSpecularAttr() const;

    /// See GetSpecularAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateSpecularAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // DIFFUSE 
    // --------------------------------------------------------------------- //
    /// Weight of the diffuse contribution.
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 1.0
    UsdAttribute GetDiffuseAttr() const;

    /// See GetDiffuseAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateDiffuseAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // INDIRECT 
    // --------------------------------------------------------------------- //
    /// Weight of the indirect contribution.
    ///
    /// \n  C++ Type: float
    /// \n  Usd Type: SdfValueTypeNames->Float
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: 1.0
    UsdAttribute GetIndirectAttr() const;

    /// See GetIndirectAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateIndirectAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ATTENUATION 
    // --------------------------------------------------------------------- //
    /// Attenuation weights. Constant, and quadratic in order.
    ///
    /// \n  C++ Type: GfVec2f
    /// \n  Usd Type: SdfValueTypeNames->Float2
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: (0, 1)
    UsdAttribute GetAttenuationAttr() const;

    /// See GetAttenuationAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateAttenuationAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // HASSHADOWS 
    // --------------------------------------------------------------------- //
    /// Enables shadow casting.
    ///
    /// \n  C++ Type: bool
    /// \n  Usd Type: SdfValueTypeNames->Bool
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: False
    UsdAttribute GetHasShadowsAttr() const;

    /// See GetHasShadowsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    UsdAttribute CreateHasShadowsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to close the class declaration with }; and complete the
    // include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--
};

#endif
