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
#include "usdKatana/attrMap.h"
#include "usdKatana/arnoldHelpers.h"
#include "usdKatana/readAiProcedural.h"
#include "usdKatana/readXformable.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdAi/aiNodeAPI.h"
#include "pxr/usd/usdAi/aiProcedural.h"
#include "pxr/usd/usdAi/aiVolume.h"

#include <FnAttribute/FnDataBuilder.h>
#include <FnLogging/FnLogging.h>

PXR_NAMESPACE_OPEN_SCOPE

FnLogSetup("PxrUsdKatanaReadAiProcedural");

void
PxrUsdKatanaReadAiProcedural(
        const UsdAiProcedural& procedural,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs)
{
    // Read in general attributes for a transformable prim.
    PxrUsdKatanaReadXformable(procedural, data, attrs);

    // Ready in Arnold visibility attributes, etc.
    FnKat::GroupAttribute arnoldStatements =
        PxrUsdKatana_GetArnoldStatementsGroup(procedural.GetPrim());
    if (arnoldStatements.isValid()) {
        attrs.set("arnoldStatements", arnoldStatements);
    }

    const double currentTime = data.GetUsdInArgs()->GetCurrentTime();

    // This plugin is registered for both AiProcedural and AiVolume, so check
    // which one we're dealing with, since the handling is slightly different.
    if (procedural.GetPrim().IsA<UsdAiVolume>()) {
        attrs.set("type", FnKat::StringAttribute("volume"));
        attrs.set("geometry.type", FnKat::StringAttribute("volumedso"));
        // TODO: Find a way to check if the "bound" attribute is already set.
        // The current plugin system doesn't give prim reader plugins any way to
        // access the ``GeolibCookInterface`` from the base PxrUsdIn op.
        attrs.set("rendererProcedural.autoBounds", FnAttribute::IntAttribute(1));

        float stepSize = 0;
        if (UsdAttribute stepAttr = UsdAiVolume(procedural).GetStepSizeAttr()) {
            stepAttr.Get<float>(&stepSize, currentTime);
        }
        attrs.set("geometry.step_size", FnKat::FloatAttribute(stepSize));
    } else {
        attrs.set("type", FnKat::StringAttribute("renderer procedural"));
    }

    // Read the DSO value.
    if (UsdAttribute dsoAttr = procedural.GetDsoAttr()) {
        // Not sure if this check is actually necessary, but this attribute
        // doesn't have a default value in the schema, so let's play it safe.
        if (dsoAttr.HasValue()) {
            std::string dso;
            dsoAttr.Get<std::string>(&dso);
            attrs.set("rendererProcedural.procedural",
                      FnKat::StringAttribute(dso));
        }
    }

    // Read all parameters in the "user:" namespace and convert their values to
    // attributes in the "rendererProcedural.args" group attribute.
    // Note that these are only sampled once per frame.
    FnKat::GroupBuilder argsBuilder;

    UsdAiNodeAPI nodeAPI = UsdAiNodeAPI(procedural);
    std::vector<UsdAttribute> userAttrs = nodeAPI.GetUserAttributes();
    TF_FOR_ALL(attrIter, userAttrs) {
        UsdAttribute userAttr = *attrIter;
        VtValue vtValue;
        if (!userAttr.Get(&vtValue, currentTime)) {
            continue;
        }

        const std::string attrBaseName = userAttr.GetBaseName().GetString();
        argsBuilder.set(
                attrBaseName,
                PxrUsdKatanaUtils::ConvertVtValueToKatAttr(vtValue, true));

        // Create KtoA hint attribute if necessary.
        std::vector<std::string> attrHints;
        if (userAttr.GetTypeName().IsArray()) {
            attrHints.push_back("array");
            attrHints.push_back("true");
        }

        std::string typeHint = PxrUsdKatana_GetArnoldAttrTypeHint(
                userAttr.GetTypeName().GetScalarType());
        if (!typeHint.empty()) {
            attrHints.push_back("type");
            attrHints.push_back(typeHint);
        }
        // TODO(?): `key_array` and `clone` hints

        if (!attrHints.empty()) {
            argsBuilder.set(std::string("arnold_hint__") + attrBaseName,
                            FnKat::StringAttribute(attrHints, 2));
        }
    }

    attrs.set("rendererProcedural.args", argsBuilder.build());
}

PXR_NAMESPACE_CLOSE_SCOPE