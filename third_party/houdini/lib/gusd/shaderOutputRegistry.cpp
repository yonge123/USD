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

#include "shaderOutputRegistry.h"

#include <pxr/base/tf/instantiateSingleton.h>

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

using _OutputRegistryElem = std::tuple<GusdShaderOutputCreator, TfToken>;
using _OutputRegistry = std::map<TfToken, _OutputRegistryElem>;
static _OutputRegistry _outputRegistry;

bool
GusdShaderOutputRegistry::registerShaderOutput(
    const std::string& name,
    const std::string& label,
    GusdShaderOutputCreator creator) {
    auto insertStatus = _outputRegistry.insert(
        {TfToken(name), _OutputRegistryElem{creator, TfToken(label)}}
    );
    return insertStatus.second;
}

GusdShaderOutputCreator
GusdShaderOutputRegistry::getShaderOutputCreator(const TfToken& name) {
    TfRegistryManager::GetInstance().SubscribeTo<GusdShaderOutput>();
    const auto it = _outputRegistry.find(name);
    return it == _outputRegistry.end() ? nullptr : std::get<0>(it->second);
}

GusdShaderOutputRegistry::ShaderOutputList
GusdShaderOutputRegistry::listOutputs() {
    TfRegistryManager::GetInstance().SubscribeTo<GusdShaderOutput>();
    ShaderOutputList ret;
    ret.reserve(_outputRegistry.size());
    for (const auto& it: _outputRegistry) {
        ret.emplace_back(it.first, std::get<1>(it.second));
    }
    return ret;
}

TF_INSTANTIATE_SINGLETON(GusdShaderOutputRegistry);

GusdShaderOutputRegistry&
GusdShaderOutputRegistry::getInstance() {
    return TfSingleton<GusdShaderOutputRegistry>::GetInstance();
}

PXR_NAMESPACE_CLOSE_SCOPE
