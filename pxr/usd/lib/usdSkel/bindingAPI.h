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
#ifndef USDSKEL_GENERATED_BINDINGAPI_H
#define USDSKEL_GENERATED_BINDINGAPI_H

/// \file usdSkel/bindingAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdSkel/tokens.h"

#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdSkel/skeleton.h" 

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// BINDINGAPI                                                                 //
// -------------------------------------------------------------------------- //

/// \class UsdSkelBindingAPI
///
/// Provides API for authoring and extracting all the skinning-related
/// data that lives in the "geometry hierarchy" of prims and models that want
/// to be skeletally deformed.
/// 
/// See the extended \ref UsdSkel_BindingAPI "UsdSkelBindingAPI schema"
/// documentation for more about bindings and how they apply in a scene graph.
/// 
///
class UsdSkelBindingAPI : public UsdAPISchemaBase
{
public:
    /// Compile-time constant indicating whether or not this class corresponds
    /// to a concrete instantiable prim type in scene description.  If this is
    /// true, GetStaticPrimDefinition() will return a valid prim definition with
    /// a non-empty typeName.
    static const bool IsConcrete = false;

    /// Compile-time constant indicating whether or not this class inherits from
    /// UsdTyped. Types which inherit from UsdTyped can impart a typename on a
    /// UsdPrim.
    static const bool IsTyped = false;

    /// Compile-time constant indicating whether or not this class represents an 
    /// applied API schema, i.e. an API schema that has to be applied to a prim
    /// with a call to auto-generated Apply() method before any schema 
    /// properties are authored.
    static const bool IsApplied = true;
    
    /// Compile-time constant indicating whether or not this class represents a 
    /// multiple-apply API schema. Mutiple-apply API schemas can be applied 
    /// to the same prim multiple times with different instance names. 
    static const bool IsMultipleApply = false;

    /// Construct a UsdSkelBindingAPI on UsdPrim \p prim .
    /// Equivalent to UsdSkelBindingAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdSkelBindingAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdSkelBindingAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdSkelBindingAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdSkelBindingAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDSKEL_API
    virtual ~UsdSkelBindingAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDSKEL_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdSkelBindingAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdSkelBindingAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDSKEL_API
    static UsdSkelBindingAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "BindingAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdSkelBindingAPI object is returned upon success. 
    /// An invalid (or empty) UsdSkelBindingAPI object is returned upon 
    /// failure. See \ref UsdAPISchemaBase::_ApplyAPISchema() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    ///
    USDSKEL_API
    static UsdSkelBindingAPI 
    Apply(const UsdPrim &prim);

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDSKEL_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDSKEL_API
    virtual const TfType &_GetTfType() const;

    // This override returns true since UsdSkelBindingAPI is an 
    // applied API schema.
    USDSKEL_API
    virtual bool _IsAppliedAPISchema() const override;

public:
    // --------------------------------------------------------------------- //
    // GEOMBINDTRANSFORM 
    // --------------------------------------------------------------------- //
    /// Encodes the transform that positions gprims in the space in
    /// which it is bound to a Skeleton.  If the transform is identical for a
    /// group of gprims that share a common ancestor, the transform may be
    /// authored on the ancestor, to "inherit" down to all the leaf gprims.
    /// The *geomBindTransform* is defined as moving a gprim from its own
    /// object space (untransformed by the gprim's own transform) out into
    /// Skeleton space. If this transform is unset, an identity transform
    /// is used instead.
    ///
    /// \n  C++ Type: GfMatrix4d
    /// \n  Usd Type: SdfValueTypeNames->Matrix4d
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetGeomBindTransformAttr() const;

    /// See GetGeomBindTransformAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateGeomBindTransformAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // JOINTS 
    // --------------------------------------------------------------------- //
    /// An (optional) array of tokens defining the list of
    /// joints to which jointIndices apply. If not defined, jointIndices applies
    /// to the ordered list of joints defined in the bound Skeleton's *joints*
    /// attribute. If undefined on a primitive, the primitive inherits the 
    /// value of the nearest ancestor prim, if any.
    ///
    /// \n  C++ Type: VtArray<TfToken>
    /// \n  Usd Type: SdfValueTypeNames->TokenArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetJointsAttr() const;

    /// See GetJointsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateJointsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // JOINTINDICES 
    // --------------------------------------------------------------------- //
    /// Indices into the *joints* attribute of the closest
    /// (in namespace) bound Skeleton that affect each point of a PointBased
    /// gprim. The primvar can have either *constant* or *vertex* interpolation.
    /// This primvar's *elementSize* will determine how many joint influences
    /// apply to each point. Indices must point be valid. Null influences should
    /// be defined by setting values in jointWeights to zero.
    /// See UsdGeomPrimvar for more information on interpolation and
    /// elementSize.
    ///
    /// \n  C++ Type: VtArray<int>
    /// \n  Usd Type: SdfValueTypeNames->IntArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetJointIndicesAttr() const;

    /// See GetJointIndicesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateJointIndicesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // JOINTWEIGHTS 
    // --------------------------------------------------------------------- //
    /// Weights for the joints that affect each point of a PointBased
    /// gprim. The primvar can have either *constant* or *vertex* interpolation.
    /// This primvar's *elementSize* will determine how many joints influences
    /// apply to each point. The length, interpolation, and elementSize of
    /// *jointWeights* must match that of *jointIndices*. See UsdGeomPrimvar
    /// for more information on interpolation and elementSize.
    ///
    /// \n  C++ Type: VtArray<float>
    /// \n  Usd Type: SdfValueTypeNames->FloatArray
    /// \n  Variability: SdfVariabilityVarying
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetJointWeightsAttr() const;

    /// See GetJointWeightsAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateJointWeightsAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // BLENDSHAPES 
    // --------------------------------------------------------------------- //
    /// An array of tokens defining the order onto which blend shape
    /// weights from an animation source map onto the *skel:blendShapeTargets*
    /// rel of a binding site. If authored, the number of elements must be equal
    /// to the number of targets in the _blendShapeTargets_ rel. This property
    /// is not inherited hierarchically, and is expected to be authored directly
    /// on the skinnable primitive to which the blend shapes apply.
    ///
    /// \n  C++ Type: VtArray<TfToken>
    /// \n  Usd Type: SdfValueTypeNames->TokenArray
    /// \n  Variability: SdfVariabilityUniform
    /// \n  Fallback Value: No Fallback
    USDSKEL_API
    UsdAttribute GetBlendShapesAttr() const;

    /// See GetBlendShapesAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDSKEL_API
    UsdAttribute CreateBlendShapesAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // ANIMATIONSOURCE 
    // --------------------------------------------------------------------- //
    /// Animation source to be bound to Skeleton primitives at or
    /// beneath the location at which this property is defined.
    /// 
    ///
    USDSKEL_API
    UsdRelationship GetAnimationSourceRel() const;

    /// See GetAnimationSourceRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDSKEL_API
    UsdRelationship CreateAnimationSourceRel() const;

public:
    // --------------------------------------------------------------------- //
    // SKELETON 
    // --------------------------------------------------------------------- //
    /// Skeleton to be bound to this prim and its descendents that
    /// possess a mapping and weighting to the joints of the identified
    /// Skeleton.
    ///
    USDSKEL_API
    UsdRelationship GetSkeletonRel() const;

    /// See GetSkeletonRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDSKEL_API
    UsdRelationship CreateSkeletonRel() const;

public:
    // --------------------------------------------------------------------- //
    // BLENDSHAPETARGETS 
    // --------------------------------------------------------------------- //
    /// Ordered list of all target blend shapes. This property is not
    /// inherited hierarchically, and is expected to be authored directly on
    /// the skinnable primitive to which the the blend shapes apply.
    ///
    USDSKEL_API
    UsdRelationship GetBlendShapeTargetsRel() const;

    /// See GetBlendShapeTargetsRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDSKEL_API
    UsdRelationship CreateBlendShapeTargetsRel() const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    /// Convenience function to get the jointIndices attribute as a primvar.
    ///
    /// \sa GetJointIndicesAttr, GetInheritedJointWeightsPrimvar
    USDSKEL_API
    UsdGeomPrimvar GetJointIndicesPrimvar() const;

    /// Convenience function to create the jointIndices primvar, optionally
    /// specifying elementSize.
    /// If \p constant is true, the resulting primvar is configured 
    /// with 'constant' interpolation, and describes a rigid deformation.
    /// Otherwise, the primvar is configured with 'vertex' interpolation,
    /// and describes joint influences that vary per point.
    ///
    /// \sa CreateJointIndicesAttr(), GetJointIndicesPrimvar()
    USDSKEL_API
    UsdGeomPrimvar CreateJointIndicesPrimvar(bool constant,
                                             int elementSize=-1) const;

    /// Convenience function to get the jointWeights attribute as a primvar.
    ///
    /// \sa GetJointWeightsAttr, GetInheritedJointWeightsPrimvar
    USDSKEL_API
    UsdGeomPrimvar GetJointWeightsPrimvar() const;

    /// Convenience function to create the jointWeights primvar, optionally
    /// specifying elementSize.
    /// If \p constant is true, the resulting primvar is configured 
    /// with 'constant' interpolation, and describes a rigid deformation.
    /// Otherwise, the primvar is configured with 'vertex' interpolation,
    /// and describes joint influences that vary per point.
    ///
    /// \sa CreateJointWeightsAttr(), GetJointWeightsPrimvar()
    USDSKEL_API
    UsdGeomPrimvar CreateJointWeightsPrimvar(bool constant,
                                             int elementSize=-1) const;

    /// Convenience method for defining joints influences that
    /// make a primitive rigidly deformed by a single joint.
    USDSKEL_API
    bool SetRigidJointInfluence(int jointIndex, float weight=1) const;

    /// Convenience method to query the Skeleton bound on this prim.
    /// Returns true if a Skeleton binding is defined, and sets \p skel to
    /// the target skel. The resulting Skeleton may still be invalid,
    /// if the Skeleton has been explicitly *unbound*.
    ///
    /// This does not resolved inherited skeleton bindings.
    USDSKEL_API
    bool GetSkeleton(UsdSkelSkeleton* skel) const;

    /// Convenience method to query the animation source bound on this prim.
    /// Returns true if an animation source binding is defined, and sets
    /// \p prim to the target prim. The resulting primitive may still be
    /// invalid, if the prim has been explicitly *unbound*.
    ///
    /// This does not resolved inherited animation source bindings.
    USDSKEL_API
    bool GetAnimationSource(UsdPrim* prim) const;

    /// Returns the skeleton bound at this prim, or one of its ancestors.
    USDSKEL_API
    UsdSkelSkeleton GetInheritedSkeleton() const;

    /// Returns the animation source bound at this prim, or one of
    /// its ancestors.
    USDSKEL_API
    UsdPrim GetInheritedAnimationSource() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
