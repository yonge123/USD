//
// Copyright 2018 Pixar
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
#ifndef __GUSD_SHADEROUTPUTREGISTRY_H__
#define __GUSD_SHADEROUTPUTREGISTRY_H__

#include <pxr/pxr.h>

#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/singleton.h>
#include <pxr/base/tf/weakPtr.h>

#include "gusd/api.h"
#include "shaderOutput.h"

#include <vector>
#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(GusdShaderOutputRegistry);
class GusdShaderOutputRegistry : public TfWeakBase
{
private:
    GusdShaderOutputRegistry() = default;
    ~GusdShaderOutputRegistry() = default;

public:
    using ShaderOutputList = std::vector<std::tuple<TfToken, TfToken>>;

    GUSD_API
    static GusdShaderOutputRegistry& getInstance();

    GUSD_API
    bool registerShaderOutput(
        const std::string& name,
        const std::string& label,
        GusdShaderOutputCreator creator);

    GUSD_API
    GusdShaderOutputCreator getShaderOutputCreator(const TfToken& name);

    GUSD_API
    ShaderOutputList listOutputs();
private:
    friend class TfSingleton<GusdShaderOutputRegistry>;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GUSD_SHADEROUTPUTREGISTRY_H__
