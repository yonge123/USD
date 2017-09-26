// Copyright 2017 Pixar
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

#ifndef PXRUSDMAYA_XFORM_STACK_H
#define PXRUSDMAYA_XFORM_STACK_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usdGeom/xformOp.h"

#include <maya/MTransformationMatrix.h>

#include <vector>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

// Note: pivotTranslate is currently not used in MayaXformStack,
// CommonXformStack, or MatrixStack, so it should never occur
// at present, but there was some support for reading it, thus
// why it's here

/// \hideinitializer
#define PXRUSDMAYA_XFORM_STACK_TOKENS \
    (translate) \
    (rotatePivotTranslate) \
    (rotatePivot) \
    (rotate) \
    (rotateAxis) \
    (scalePivotTranslate) \
    (scalePivot) \
    (shear) \
    (scale) \
    (pivot) \
    (pivotTranslate) \
    (transform)

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaXformStackTokens,
        PXRUSDMAYA_API,
        PXRUSDMAYA_XFORM_STACK_TOKENS);

/// \class PxrUsdMayaXformOpClassification
/// \brief Defines a named "class" of xform operation
///
/// Similar to UsdGeomXformOp, but without a specific attribute;
/// UsdGeomXformOps can be thought of as "instances" of a
/// PxrUsdMayaXformOpDefinition "type"
class PxrUsdMayaXformOpClassification
{
public:
    PXRUSDMAYA_API
    TfToken const &GetName() const {
        return _name;
    };

    PXRUSDMAYA_API
    UsdGeomXformOp::Type GetOpType() const {
        return _opType;
    };

    PXRUSDMAYA_API
    bool IsInvertedTwin() const {
        return _isInvertedTwin;
    };

    /// Return True if the given op type is compatible with this
    /// OpClassification (ie, is the same, or is, say rotateX, when
    /// this op type is rotateXYZ)
    PXRUSDMAYA_API
    bool IsCompatibleType(UsdGeomXformOp::Type otherType) const;

    PXRUSDMAYA_API
    bool operator ==(const PxrUsdMayaXformOpClassification& other) const;

    PXRUSDMAYA_API
    std::vector<TfToken> CompatibleAttrNames() const;

    friend class PxrUsdMayaXformStack;
private:
    // Private because we currently want the only instances of this class to
    // be stored in MayaStack and CommonStack, defined at compile
    // time
    PxrUsdMayaXformOpClassification(const TfToken &name,
                                       UsdGeomXformOp::Type opType,
                                       bool isInvertedTwin=false);

    const TfToken _name;
    const UsdGeomXformOp::Type _opType;
    const bool _isInvertedTwin;
};

/// \class PxrUsdMayaXformStack
/// \brief Defines a standard list of xform operations.
///
/// Used to define the set and order of transforms that programs like
/// Maya use and understand.
class PxrUsdMayaXformStack
{
public:
    typedef std::vector<PxrUsdMayaXformOpClassification> OpClassList;

    // Note that OpClassPtr is a raw, non-reference-counted pointer; this should be fine,
    // as all PxrUsdMayaXformOpClassification are statically created, and should never
    // be deleted, so we don't need to worry about pointers to them going out of scope.
    // If at some point this changes, and we allow user-created OpClassifications, we
    // may consider using reference-counted pointers (though that will have a small
    // extra cost.)
    typedef const PxrUsdMayaXformOpClassification* OpClassPtr;
    typedef std::pair<OpClassPtr, OpClassPtr> OpClassPtrPair;
    typedef std::unordered_map<TfToken, OpClassPtrPair, TfToken::HashFunctor>
            TokenPtrPairMap;
    typedef std::unordered_map<size_t, size_t> IndexMap;

    PXRUSDMAYA_API
    const std::vector<PxrUsdMayaXformOpClassification>& GetOps() const {
        return _ops;
    };

    PXRUSDMAYA_API
    const std::vector<std::pair<size_t, size_t> >& GetInversionTwins() const {
        return _inversionTwins;
    };

    PXRUSDMAYA_API
    bool GetNameMatters() const {
        return _nameMatters;
    };

    PXRUSDMAYA_API
    const PxrUsdMayaXformOpClassification& operator[] (const size_t index) const {
        return _ops[index];
    }

    /// \brief  Finds the Op Classification with the given name in this stack
    /// \param  opName the name of the operator classification we  wish to find
    /// \param  isInvertedTwin the returned op classification object must match
    ///         this param for it's IsInvertedTwin() - if an op is found that matches
    ///         the name, but has the wrong invertedTwin status, nullptr is returned
    /// return  Pointer to the op classification object with the given name (and
    ///         inverted twin state); will be nullptr if no match could be found.
    PXRUSDMAYA_API
    OpClassPtr FindOp(const TfToken& opName, bool isInvertedTwin=false) const;

    /// \brief  Finds the Op Classification(s) with the given name in this stack
    /// \param  opName the name of the operator classification we  wish to find
    /// return  A pair of pointers to op classification objects with the given name;
    ///         if the objects are part of an inverted twin pair, then both are
    ///         returned (in the order they appear in this stack). If found, but
    ///         not as part of an inverted twin pair, the first result will point
    ///         to the found classification, and the second will be null.  If
    ///         no matches are found, both pointers will be nullptr.
    PXRUSDMAYA_API
    const OpClassPtrPair& FindOpPair(const TfToken& opName) const;

    /// \brief Returns a list of pointers to matching XformOpDefinitions for this stack
    ///
    /// For each xformop, we want to find the corresponding op within this
    /// stack that it matches.  There are 3 requirements:
    ///  - to be considered a match, the name and type must match an op in this stack
    ///  - the matches for each xformop must have increasing indexes in the stack
    ///  - inversionTwins must either both be matched or neither matched.
    ///
    /// This returns a vector of pointers to the matching XformOpDefinitions in this
    /// stack. The size of this vector will be 0 if no complete match is found,
    /// or xformops.size() if a complete match is found.
    PXRUSDMAYA_API
    std::vector<OpClassPtr>
    MatchingSubstack(
            const std::vector<UsdGeomXformOp>& xformops,
            MTransformationMatrix::RotationOrder* MrotOrder=nullptr) const;

    /// \brief The standard Maya xform stack
    ///
    /// Consists of these xform operators:
    ///    translate
    ///    rotatePivotTranslate
    ///    rotatePivot
    ///    rotate
    ///    rotateAxis
    ///    rotatePivot^-1 (inverted twin)
    ///    scalePivotTranslate
    ///    scalePivot
    ///    shear
    ///    scale
    ///    scalePivot^-1 (inverted twin)
    PXRUSDMAYA_API
    static const PxrUsdMayaXformStack& MayaStack();


    /// \brief The Common API xform stack
    ///
    /// Consists of these xform operators:
    ///    translate
    ///    pivot
    ///    rotate
    ///    scale
    ///    pivot^-1 (inverted twin)
    PXRUSDMAYA_API
    static const PxrUsdMayaXformStack& CommonStack();

    /// \brief xform "stack" consisting of only a single matrix xform
    ///
    /// This stack will match any list of xform ops that consist of a single
    /// matrix "transform" op, regardless of name.
    /// Consists of these xform operators:
    ///    transform
    PXRUSDMAYA_API
    static const PxrUsdMayaXformStack& MatrixStack();

    /// \brief dummy recursion endpoint for FirstMatchingSubstack variadic template
    PXRUSDMAYA_API
    inline static std::vector<OpClassPtr>
    FirstMatchingSubstack(
            const std::vector<UsdGeomXformOp>& xformops,
            MTransformationMatrix::RotationOrder* MrotOrder=nullptr)
    {
        return std::vector<OpClassPtr>();
    }

    /// \brief Runs MatchingSubstack against the given list of stacks
    ///
    /// Returns the first non-empty result it finds; if all stacks
    /// return an empty vector, an empty vector is returned.
    template<typename... RemainingTypes>
    static std::vector<OpClassPtr>
    FirstMatchingSubstack(
            const std::vector<UsdGeomXformOp>& xformops,
            MTransformationMatrix::RotationOrder* MrotOrder,
            const PxrUsdMayaXformStack& firstStack,
            RemainingTypes&... otherStacks)
    {
        if (xformops.empty()) return std::vector<OpClassPtr>();

        std::vector<PxrUsdMayaXformStack::OpClassPtr> stackOps = \
                firstStack.MatchingSubstack(xformops, MrotOrder);
        if (!stackOps.empty())
        {
            return stackOps;
        }
        return FirstMatchingSubstack(xformops, MrotOrder, otherStacks...);
    }

private:
    // Private because we currently want the only instances of this class to
    // be const static members, MayaStack and CommonStack, defined at compile
    // time
    PxrUsdMayaXformStack(
            const OpClassList ops,
            const std::vector<std::pair<size_t, size_t> > inversionTwins,
            bool nameMatters=true);

    // Don't want to accidentally make a copy, since the only instances are supposed
    // to be static!
    PxrUsdMayaXformStack(const PxrUsdMayaXformStack& other) = delete;

    OpClassList _ops;
    std::vector<std::pair<size_t, size_t> > _inversionTwins;
    IndexMap _inversionMap;

    // We store lookups from raw attribute name - use full attribute
    // name because it's the only "piece" we know we have a pre-generated
    // TfToken for - even Property::GetBaseName() generates a new TfToken
    // "on the fly".
    // The lookup maps to a PAIR of pointers into the ops list;
    // we return a pair because, due to inversion twins, it's possible
    // for there to be two (but there should be only two!) ops with
    // the same name - ie, if they're inversion twins. Thus, each pair
    // of ptrs will either be:
    //     { opPtr, nullptr }      if opPtr has no inversion twin
    // or
    //     { opPtr, opPtrTwin }    if opPtr has an inversion twin
    TokenPtrPairMap _attrNamesToPtrs;

    // Also have a lookup by op name, for use by FindOp
    TokenPtrPairMap _opNamesToPtrs;

    bool _nameMatters = true;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_XFORM_STACK_H
