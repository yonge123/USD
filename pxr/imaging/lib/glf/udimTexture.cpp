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
/// \file glf/udimTexture.cpp

#include "pxr/imaging/glf/udimTexture.h"
#include "pxr/imaging/glf/glew.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

bool GlfIsSupportedUdimTexture(const std::string& imageFilePath) {
    return TfStringContains(imageFilePath, "<udim>") ||
           TfStringContains(imageFilePath, "<UDIM>");
}

// TODO: improve and optimize this function!
std::vector<std::tuple<int, std::string>> GlfGetUdimTiles(const std::string& imageFilePath) {
    auto pos = imageFilePath.find("<udim>");
    if (pos == std::string::npos) {
        pos = imageFilePath.find("<UDIM>");
    }

    if (pos == std::string::npos) { return {}; }

    const auto start = imageFilePath.substr(0, pos);
    const auto end = imageFilePath.substr(pos + 6);

    std::vector<std::tuple<int, std::string>> ret;
    for (auto t = 1001; t <= 1100; ++t) {
        std::stringstream ss; ss << start << t << end;
        if (TfPathExists(ss.str())) {
            ret.push_back(std::tuple<int, std::string>(t, ss.str()));
        }
    }

    return ret;
}


PXR_NAMESPACE_CLOSE_SCOPE
