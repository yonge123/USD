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

#include "usdMaya/xformStack.h"

#include "pxr/base/tf/stringUtils.h"

#include <exception>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaXformStackTokens,
        PXRUSDMAYA_XFORM_STACK_TOKENS);

typedef PxrUsdMayaXformStack::OpClassList OpClassList;
typedef PxrUsdMayaXformStack::OpClassPtr OpClassPtr;
typedef PxrUsdMayaXformStack::OpClassPtrPair OpClassPtrPair;
typedef PxrUsdMayaXformStack::TokenPtrPairMap TokenPtrPairMap;
typedef PxrUsdMayaXformStack::IndexMap IndexMap;

namespace {
    const UsdGeomXformOp::Type RotateOpTypes[] = {
        UsdGeomXformOp::TypeRotateX,
        UsdGeomXformOp::TypeRotateY,
        UsdGeomXformOp::TypeRotateZ,
        UsdGeomXformOp::TypeRotateXYZ,
        UsdGeomXformOp::TypeRotateXZY,
        UsdGeomXformOp::TypeRotateYXZ,
        UsdGeomXformOp::TypeRotateYZX,
        UsdGeomXformOp::TypeRotateZXY,
        UsdGeomXformOp::TypeRotateZYX
    };

    bool _isThreeAxisRotate(UsdGeomXformOp::Type opType)
    {
        return (opType == UsdGeomXformOp::TypeRotateXYZ
                || opType == UsdGeomXformOp::TypeRotateXZY
                || opType == UsdGeomXformOp::TypeRotateYXZ
                || opType == UsdGeomXformOp::TypeRotateYZX
                || opType == UsdGeomXformOp::TypeRotateZXY
                || opType == UsdGeomXformOp::TypeRotateZYX);
    }

    bool _isOneOrThreeAxisRotate(UsdGeomXformOp::Type opType)
    {
        return (opType == UsdGeomXformOp::TypeRotateXYZ
                || opType == UsdGeomXformOp::TypeRotateXZY
                || opType == UsdGeomXformOp::TypeRotateYXZ
                || opType == UsdGeomXformOp::TypeRotateYZX
                || opType == UsdGeomXformOp::TypeRotateZXY
                || opType == UsdGeomXformOp::TypeRotateZYX
                || opType == UsdGeomXformOp::TypeRotateX
                || opType == UsdGeomXformOp::TypeRotateY
                || opType == UsdGeomXformOp::TypeRotateZ);
    }

    IndexMap _buildInversionMap(
            const std::vector<std::pair<size_t, size_t> >& inversionTwins)
    {
        IndexMap result;
        for (const auto& twinPair : inversionTwins)
        {
            result[twinPair.first] = twinPair.second;
            result[twinPair.second] = twinPair.first;
        }
        return result;
    }

    // Given a single pointer into ops, return the pair of
    // pointers, which is:
    //    {opPtr, nullptr}    if opPtr does not have an inverted twin
    //    {opPtr, opPtrTwin}  if opPtr does have an inverted twin
    OpClassPtrPair
    _makeInversionPtrPair(
            const OpClassPtr opPtr,
            const OpClassList& ops,
            const IndexMap& inversionMap)
    {
        auto foundTwin = inversionMap.find(opPtr - &ops[0]);
        if (foundTwin == inversionMap.end())
        {
            return std::make_pair(opPtr, nullptr);
        }
        else
        {
            OpClassPtr twinOpPtr = &ops[foundTwin->second];
            if (twinOpPtr >= opPtr)
            {
                return std::make_pair(opPtr, twinOpPtr);
            }
            else
            {
                return std::make_pair(twinOpPtr, opPtr);
            }
        }
    }

    TokenPtrPairMap _buildAttrNamesToPtrs(
            const OpClassList& ops,
            const IndexMap& inversionMap)
    {
        TokenPtrPairMap result;
        for (auto& op: ops)
        {
            // Inverted twin pairs will always have same name, so can skip one
            if (op.IsInvertedTwin()) continue;

            for (auto attrName : op.CompatibleAttrNames())
            {
                auto ptrPair = _makeInversionPtrPair(&op, ops, inversionMap);
                // Insert, and check if it already existed
                if (!result.insert({attrName, ptrPair}).second)
                {
                    // The attrName was already in the lookup map...
                    // ...this shouldn't happen, error
                    std::string msg = TfStringPrintf(
                            "AttrName %s already found in attrName lookup map",
                            attrName.GetText());
                    throw std::invalid_argument(msg);
                }
            }
        }
        return result;
    }

    TokenPtrPairMap _buildOpNamesToPtrs(
            const OpClassList& ops,
            const IndexMap& inversionMap)
    {
        TokenPtrPairMap result;
        for (auto& op: ops)
        {
            // Inverted twin pairs will always have same name, so can skip one
            if (op.IsInvertedTwin()) continue;

            auto ptrPair = _makeInversionPtrPair(&op, ops, inversionMap);
            // Insert, and check if it already existed
            if (!result.insert({op.GetName(), ptrPair}).second)
            {
                // The op name was already in the lookup map...
                // ...this shouldn't happen, error
                std::string msg = TfStringPrintf(
                        "Op classification name %s already found in op lookup map",
                        op.GetName().GetText());
                throw std::invalid_argument(msg);
            }
        }
        return result;
    }
}

PxrUsdMayaXformOpClassification::PxrUsdMayaXformOpClassification(
        const TfToken &name,
        UsdGeomXformOp::Type opType,
        bool isInvertedTwin) :
                _name(name),
                _opType(opType),
                _isInvertedTwin(isInvertedTwin)
{
}

std::vector<TfToken>
PxrUsdMayaXformOpClassification::CompatibleAttrNames() const
{
    // Note that we make tokens immortal because PxrUsdMayaXformOpClassification
    // is currently not publically creatable, and is only used to make
    // some global constants (ie, MayaStack and CommonStack)
    std::vector<TfToken> result;
    std::string attrName;
    if (_name == PxrUsdMayaXformStackTokens->rotate
            && _isThreeAxisRotate(_opType))
    {
        result.reserve(std::extent<decltype(RotateOpTypes)>::value * 3);
        // Special handling for rotate, to deal with rotateX/rotateZXY/etc
        for (UsdGeomXformOp::Type rotateType : RotateOpTypes)
        {
            // Add, ie, xformOp::rotateX
            result.emplace_back(TfToken(
                    UsdGeomXformOp::GetOpName(rotateType).GetString(),
                    TfToken::Immortal));
            // Add, ie, xformOp::rotateX::rotate
            result.emplace_back(TfToken(
                    UsdGeomXformOp::GetOpName(rotateType,
                            PxrUsdMayaXformStackTokens->rotate).GetString(),
                    TfToken::Immortal));
            // Add, ie, xformOp::rotateX::rotateX
            result.emplace_back(TfToken(
                    UsdGeomXformOp::GetOpName(rotateType,
                            UsdGeomXformOp::GetOpTypeToken(rotateType)).GetString(),
                    TfToken::Immortal));
        }
    }
    else
    {
        // Add, ie, xformOp::translate::someName
        result.emplace_back(TfToken(
                UsdGeomXformOp::GetOpName(_opType, _name).GetString(),
                TfToken::Immortal));
        if (_name == UsdGeomXformOp::GetOpTypeToken(_opType))
        {
            // Add, ie, xformOp::translate
            result.emplace_back(TfToken(
                    UsdGeomXformOp::GetOpName(_opType).GetString(),
                    TfToken::Immortal));
        }
    }
    return result;
}

bool
PxrUsdMayaXformOpClassification::IsCompatibleType(UsdGeomXformOp::Type otherType) const
{
    if (_opType == otherType) return true;
    if (_isThreeAxisRotate(_opType ))
    {
        return _isOneOrThreeAxisRotate(otherType);
    }
    return false;
}

bool PxrUsdMayaXformOpClassification::operator ==(const PxrUsdMayaXformOpClassification& other) const
{
    return (_name == other._name && _opType == other._opType && _isInvertedTwin == other._isInvertedTwin);
}

PxrUsdMayaXformStack::PxrUsdMayaXformStack(
        const OpClassList ops,
        const std::vector<std::pair<size_t, size_t> > inversionTwins,
        bool nameMatters) :
                _ops(ops),
                _inversionTwins(inversionTwins),
                _inversionMap(_buildInversionMap(inversionTwins)),
                _attrNamesToPtrs(
                        _buildAttrNamesToPtrs(_ops, _inversionMap)),
                _opNamesToPtrs(
                        _buildOpNamesToPtrs(_ops, _inversionMap)),
                _nameMatters(nameMatters)
{
    // Verify that all inversion twins are of same type, and exactly one is marked
    // as the inverted twin
    for (auto& pair : _inversionTwins)
    {
        const PxrUsdMayaXformOpClassification& first = _ops[pair.first];
        const PxrUsdMayaXformOpClassification& second = _ops[pair.second];
        if (first.GetName() != second.GetName())
        {
            std::string msg = TfStringPrintf(
                    "Inversion twins %lu (%s) and %lu (%s) did not have same name",
                    pair.first, first.GetName().GetText(),
                    pair.second, second.GetName().GetText());
            throw std::invalid_argument(msg);
        }
        if (first.GetOpType() != second.GetOpType())
        {
            std::string msg = TfStringPrintf(
                    "Inversion twins %lu and %lu (%s) were not same op type",
                    pair.first, pair.second, first.GetName().GetText());
            throw std::invalid_argument(msg);
        }
        if (first.IsInvertedTwin() == second.IsInvertedTwin())
        {
            if (first.IsInvertedTwin())
            {
                std::string msg = TfStringPrintf(
                        "Inversion twins %lu and %lu (%s) were both marked as the inverted twin",
                        pair.first, pair.second, first.GetName().GetText());
                throw std::invalid_argument(msg);
            }
            else
            {
                std::string msg = TfStringPrintf(
                        "Inversion twins %lu and %lu (%s) were both marked as not the inverted twin",
                        pair.first, pair.second, first.GetName().GetText());
                throw std::invalid_argument(msg);
            }
        }
    }
}

OpClassPtr
PxrUsdMayaXformStack::FindOp(const TfToken& opName, bool isInvertedTwin) const
{
    const OpClassPtrPair& foundPtrPair = FindOpPair(opName);

    if(foundPtrPair.first == nullptr) return nullptr;

    // we (potentially) found a pair of opPtrs... use the one that's
    // not the inverted twin

    if (foundPtrPair.first->IsInvertedTwin())
    {
        if (isInvertedTwin) return foundPtrPair.first;
        else return foundPtrPair.second;
    }
    else
    {
        if (isInvertedTwin) return foundPtrPair.second;
        else return foundPtrPair.first;
    }
}

const OpClassPtrPair&
PxrUsdMayaXformStack::FindOpPair(const TfToken& opName) const
{
    static OpClassPtrPair _NO_MATCH(nullptr, nullptr);

    TokenPtrPairMap::const_iterator foundTokenPtrPair =
            _opNamesToPtrs.find(opName);
    if (foundTokenPtrPair == _opNamesToPtrs.end())
    {
        // Couldn't find the xformop in our stack, abort
        return _NO_MATCH;
    }
    return foundTokenPtrPair->second;
}

std::vector<OpClassPtr>
PxrUsdMayaXformStack::MatchingSubstack(
        const std::vector<UsdGeomXformOp>& xformops,
        MTransformationMatrix::RotationOrder* MrotOrder) const
{
    static const std::vector<OpClassPtr> _NO_MATCH;

    if (xformops.empty()) return _NO_MATCH;

    std::vector<OpClassPtr> ret;

    // nextOp keeps track of where we will start looking for matches.  It
    // will only move forward.
    auto nextOp = _ops.cbegin();

    std::vector<bool> opNamesFound(_ops.size(), false);

    TF_FOR_ALL(iter, xformops) {
        const UsdGeomXformOp& xformOp = *iter;
        OpClassPtr foundOp = nullptr;

        if(_nameMatters) {
            // First try the fast attrName lookup...
            TokenPtrPairMap::const_iterator foundTokenPtrPair =
                    _attrNamesToPtrs.find(xformOp.GetName());
            if (foundTokenPtrPair == _attrNamesToPtrs.end())
            {
                // Couldn't find the xformop in our stack, abort
                return _NO_MATCH;
            }

            // we found a pair of opPtrs... make sure one is
            // not less than nextOp...
            const OpClassPtrPair& foundPtrPair = foundTokenPtrPair->second;

            if (foundPtrPair.first >= &(*nextOp))
            {
                foundOp = foundPtrPair.first;
            }
            else if (foundPtrPair.second >= &(*nextOp))
            {
                foundOp = foundPtrPair.second;
            }
            else
            {
                // The result we found is before an earlier-found op,
                // so it doesn't match our stack... abort.
                return _NO_MATCH;
            }

            // Now check that the op type matches...
            if (!foundOp->IsCompatibleType(xformOp.GetOpType()))
            {
                return _NO_MATCH;
            }
        }
        else {
            // If name does matter, we just iterate through the remaining ops, until
            // we find one with a matching type...
            for(auto& opClassIt = nextOp; opClassIt < _ops.cend(); ++opClassIt)
            {
                if (opClassIt->IsCompatibleType(xformOp.GetOpType()))
                {
                    foundOp = &(*opClassIt);
                    break;
                }
            }
            if (!foundOp) return _NO_MATCH;
        }

        // Ok, we found a match...

        // if we're a rotate, set the maya rotation order (if it's relevant to
        // this op)
        if (foundOp->GetName() == PxrUsdMayaXformStackTokens->rotate
                && MrotOrder) {
            switch(xformOp.GetOpType()) {
                case UsdGeomXformOp::TypeRotateXYZ:
                    *MrotOrder = MTransformationMatrix::kXYZ;
                break;
                case UsdGeomXformOp::TypeRotateXZY:
                    *MrotOrder = MTransformationMatrix::kXZY;
                break;
                case UsdGeomXformOp::TypeRotateYXZ:
                    *MrotOrder = MTransformationMatrix::kYXZ;
                break;
                case UsdGeomXformOp::TypeRotateYZX:
                    *MrotOrder = MTransformationMatrix::kYZX;
                break;
                case UsdGeomXformOp::TypeRotateZXY:
                    *MrotOrder = MTransformationMatrix::kZXY;
                break;
                case UsdGeomXformOp::TypeRotateZYX:
                    *MrotOrder = MTransformationMatrix::kZYX;
                break;

                default: break;
            }
        }

        // move the nextOp pointer along.
        ret.push_back(foundOp);
        int nextOpNameIndex = foundOp - &_ops[0];
        opNamesFound[nextOpNameIndex] = true;
        nextOp = _ops.cbegin() + nextOpNameIndex + 1;
    }

    // check pivot pairs
    TF_FOR_ALL(pairIter, _inversionTwins) {
        if (opNamesFound[pairIter->first] != opNamesFound[pairIter->second]) {
            return _NO_MATCH;
        }
    }

    return ret;
}

const PxrUsdMayaXformStack& PxrUsdMayaXformStack::MayaStack()
{
    static PxrUsdMayaXformStack mayaStack(
            // ops
            {
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->translate,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotatePivotTranslate,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotatePivot,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotate,
                        UsdGeomXformOp::TypeRotateXYZ),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotateAxis,
                        UsdGeomXformOp::TypeRotateXYZ),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotatePivot,
                        UsdGeomXformOp::TypeTranslate,
                        true /* isInvertedTwin */),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->scalePivotTranslate,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->scalePivot,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->shear,
                        UsdGeomXformOp::TypeTransform),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->scale,
                        UsdGeomXformOp::TypeScale),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->scalePivot,
                        UsdGeomXformOp::TypeTranslate,
                        true /* isInvertedTwin */)
            },

            // inversionTwins
            {
                {2, 5},
                {7, 10},
            });

    return mayaStack;
}

const PxrUsdMayaXformStack& PxrUsdMayaXformStack::CommonStack()
{
    static PxrUsdMayaXformStack commonStack(
            // ops
            {
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->translate,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->pivot,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotate,
                        UsdGeomXformOp::TypeRotateXYZ),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->scale,
                        UsdGeomXformOp::TypeScale),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->pivot,
                        UsdGeomXformOp::TypeTranslate,
                        true /* isInvertedTwin */)
            },

            // inversionTwins
            {
                {1, 4},
            });

    return commonStack;
}

const PxrUsdMayaXformStack& PxrUsdMayaXformStack::MatrixStack()
{
    static PxrUsdMayaXformStack matrixStack(
            // ops
            {
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->transform,
                        UsdGeomXformOp::TypeTransform)
            },

            // inversionTwins
            {
            },

            // nameMatters
            false
    );

    return matrixStack;
}

PXR_NAMESPACE_CLOSE_SCOPE
