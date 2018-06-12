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

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/udimTexture.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/trace/trace.h"

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

TF_REGISTRY_FUNCTION(TfType)
{
    typedef GlfUdimTexture Type;
    TfType t = TfType::Define<Type, TfType::Bases<GlfTexture> >();
    t.SetFactory< GlfTextureFactory<Type> >();
}

GlfUdimTexture::GlfUdimTexture(const TfToken& imageFilePath)
    : _imagePath(imageFilePath) {

}

GlfUdimTexture::~GlfUdimTexture() {
    _FreeTextureObject();
}

GlfUdimTextureRefPtr
GlfUdimTexture::New(const TfToken& imageFilePath) {
    return TfCreateRefPtr(new GlfUdimTexture(imageFilePath));
}

GlfTexture::BindingVector
GlfUdimTexture::GetBindings(
    const TfToken& identifier,
    GLuint samplerName) const {
    BindingVector ret;

    ret.push_back(Binding(
        TfToken(identifier.GetString() + "_Images"), GlfTextureTokens->texels,
        GL_TEXTURE_2D_ARRAY, _imageArray, samplerName));

    return ret;
}

VtDictionary
GlfUdimTexture::GetTextureInfo() const {
    VtDictionary ret;

    ret["memoryUsed"] = GetMemoryUsed();
    ret["width"] = (int)_width;
    ret["height"] = (int)_height;
    ret["depth"] = (int)_depth;
    ret["format"] = (int)_format;
    ret["imageFilePath"] = _imagePath;
    ret["referenceCount"] = GetRefCount().Get();

    return ret;
}


void
GlfUdimTexture::_FreeTextureObject() {
    GlfSharedGLContextScopeHolder sharedGLContextScopeHolder;

    if (glIsTexture(_imageArray)) {
        glDeleteTextures(1, &_imageArray);
    }
}


void
GlfUdimTexture::_OnSetMemoryRequested(size_t targetMemory) {
    _ReadImage(targetMemory);
}

void
GlfUdimTexture::_ReadImage(size_t targetMemory) {
    std::cerr << "Reading udim image: " << _imagePath << std::endl;
    TRACE_FUNCTION();
    _FreeTextureObject();

    const auto numChannels = 4;
    const auto type = GL_UNSIGNED_BYTE;

    _format = GL_RGBA8;
    auto sizePerElem = 1;
    if (type == GL_FLOAT) {
        constexpr GLenum floatFormats[] = {
            GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F
        };
        _format = floatFormats[numChannels - 1];
        sizePerElem = 4;
    } else if (type == GL_UNSIGNED_SHORT) {
        constexpr GLenum uint16Formats[] = {
            GL_R16, GL_RG16, GL_RGB16, GL_RGBA16
        };
        _format = uint16Formats[numChannels - 1];
        sizePerElem = 2;
    } else if (type == GL_HALF_FLOAT_ARB) {
        constexpr GLenum halfFormats[] = {
            GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F
        };
        _format = halfFormats[numChannels - 1];
        sizePerElem = 2;
    } else if (type == GL_UNSIGNED_BYTE) {
        constexpr GLenum uint8Formats[] = {
            GL_R8, GL_RG8, GL_RGB8, GL_RGBA8
        };
        _format = uint8Formats[numChannels - 1];
        sizePerElem = 1;
    }

    _width = 1;
    _height = 1;
    _depth = 100;

    glGenTextures(1, &_imageArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _imageArray);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<uint8_t> textureData;
    textureData.resize(_width * _height * _depth * numChannels, 255);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0,
                 _format,
                 _width,
                 _height,
                 _depth,
                 0, _format, type, textureData.data());

    GLF_POST_PENDING_GL_ERRORS();

    _SetMemoryUsed(_width * _height * _depth * sizePerElem * numChannels);
}

PXR_NAMESPACE_CLOSE_SCOPE
