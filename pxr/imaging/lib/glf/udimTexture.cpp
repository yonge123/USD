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
#include "pxr/imaging/glf/image.h"

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
std::vector<std::tuple<int, TfToken>> GlfGetUdimTiles(const std::string& imageFilePath) {
    auto pos = imageFilePath.find("<udim>");
    if (pos == std::string::npos) {
        pos = imageFilePath.find("<UDIM>");
    }

    if (pos == std::string::npos) { return {}; }
    auto formatString = imageFilePath; formatString.replace(pos, 6, "%i");

    std::vector<std::tuple<int, TfToken>> ret;
    ret.reserve(100);
    for (auto t = 1001; t <= 1100; ++t) {
        const auto path = TfStringPrintf(formatString.c_str(), t);
        if (TfPathExists(path)) {
            ret.emplace_back(t, TfToken(path));
        }
    }
    ret.shrink_to_fit();

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
    ret["format"] = _format;
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
    TRACE_FUNCTION();
    _FreeTextureObject();

    const auto numChannels = 4;
    const auto type = GL_UNSIGNED_BYTE;

    constexpr GLenum formats[] = {
        GL_RED, GL_RG, GL_RGB, GL_RGBA
    };
    _format = formats[numChannels - 1];
    auto sizePerElem = 1;
    /*if (type == GL_FLOAT) {
        sizePerElem = 4;
    } else if (type == GL_UNSIGNED_SHORT) {
        sizePerElem = 2;
    } else if (type == GL_HALF_FLOAT_ARB) {
        sizePerElem = 2;
    } else if (type == GL_UNSIGNED_BYTE) {
        sizePerElem = 1;
    }*/

    _width = 128;
    _height = 128;
    _depth = 100;

    glGenTextures(1, &_imageArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _imageArray);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<uint8_t> textureData;
    const auto numPixels = _width * _height * _depth;
    const auto numBytes = numPixels * sizePerElem * numChannels;
    textureData.resize(numBytes, 0);
    // Setting up the texture data for debugging.
    for (auto i = decltype(numPixels){0}; i < numPixels; ++i) {
        textureData[i * numChannels] = 255;
        textureData[i * numChannels + numChannels - 1] = 255;
    }

    for (const auto& tile: GlfGetUdimTiles(_imagePath)) {
        auto image = GlfImage::OpenForReading(std::get<1>(tile));
        if (image) {
            GlfImage::StorageSpec spec;
            spec.width = static_cast<int>(_width);
            spec.height = static_cast<int>(_height);
            spec.format = GL_RGBA;
            spec.type = GL_UNSIGNED_BYTE;
            spec.flipped = false;
            spec.data = textureData.data() + (std::get<0>(tile) - 1001)
                * _width * _height * sizePerElem * numChannels;
            image->Read(spec);
        }
    }

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0,
                 numChannels,
                 static_cast<GLsizei>(_width),
                 static_cast<GLsizei>(_height),
                 static_cast<GLsizei>(_depth),
                 0, GL_RGBA, type, textureData.data());

    GLF_POST_PENDING_GL_ERRORS();

    _SetMemoryUsed(numBytes);
}

PXR_NAMESPACE_CLOSE_SCOPE
