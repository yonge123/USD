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

namespace {

// TODO: improve and optimize this function!
std::vector<std::tuple<int, TfToken>>
GlfGetUdimTiles(const std::string& imageFilePath, int maxLayerCount) {
    auto pos = imageFilePath.find("<udim>");
    if (pos == std::string::npos) {
        pos = imageFilePath.find("<UDIM>");
    }

    if (pos == std::string::npos) { return {}; }
    auto formatString = imageFilePath; formatString.replace(pos, 6, "%i");

    std::vector<std::tuple<int, TfToken>> ret;
    constexpr auto startTile = 1001;
    const auto endTile = startTile + maxLayerCount;
    ret.reserve(endTile - startTile + 1);
    for (auto t = startTile; t <= endTile; ++t) {
        const auto path = TfStringPrintf(formatString.c_str(), t);
        if (TfPathExists(path)) {
            ret.emplace_back(t - startTile, TfToken(path));
        }
    }
    ret.shrink_to_fit();

    return ret;
}

}

bool GlfIsSupportedUdimTexture(const std::string& imageFilePath) {
    return TfStringContains(imageFilePath, "<udim>") ||
           TfStringContains(imageFilePath, "<UDIM>");
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

    // This is 2048 OGL 4.5
    int maxArrayTextureLayers = 0;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayTextureLayers);
    int max3dTextureSize = 0;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3dTextureSize);

    const auto tiles = GlfGetUdimTiles(_imagePath, maxArrayTextureLayers);
    if (tiles.empty()) {
        return;
    }

    auto firstImage = GlfImage::OpenForReading(std::get<1>(tiles[0]));
    if (!firstImage) {
        return;
    }

    // TODO: LUMA check for mipmaps
    _format = firstImage->GetFormat();
    const auto type = firstImage->GetType();
    int numChannels;
    if (_format == GL_RED || _format == GL_LUMINANCE) {
        numChannels = 1;
    } else if (_format == GL_RG) {
        numChannels = 2;
    } else if (_format == GL_RGB) {
        numChannels = 3;
    } else if (_format == GL_RGBA) {
        numChannels = 4;
    } else {
        return;
    }

    auto _internalFormat = GL_RGBA8;
    auto sizePerElem = 1;
    if (type == GL_FLOAT) {
        constexpr GLenum internalFormats[] =
            { GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F };
        _internalFormat = internalFormats[numChannels - 1];
        sizePerElem = 4;
    } else if (type == GL_UNSIGNED_SHORT) {
        constexpr GLenum internalFormats[] =
            { GL_R16, GL_RG16, GL_RGB16, GL_RGBA16 };
        _internalFormat = internalFormats[numChannels - 1];
        sizePerElem = 2;
    } else if (type == GL_HALF_FLOAT_ARB) {
        constexpr GLenum internalFormats[] =
            { GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F };
        _internalFormat = internalFormats[numChannels - 1];
        sizePerElem = 2;
    } else if (type == GL_UNSIGNED_BYTE) {
        constexpr GLenum internalFormats[] =
            { GL_R8, GL_RG8, GL_RGB8, GL_RGBA8 };
        _internalFormat = internalFormats[numChannels - 1];
        sizePerElem = 1;
    }

    _width = 128;
    _height = 128;
    // TODO: LUMA sparse storage
    _depth = static_cast<size_t>(std::get<0>(tiles.back()) + 1);

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

    for (const auto& tile: tiles) {
        auto image = GlfImage::OpenForReading(std::get<1>(tile));
        if (image) {
            GlfImage::StorageSpec spec;
            spec.width = static_cast<int>(_width);
            spec.height = static_cast<int>(_height);
            spec.format = _format;
            spec.type = type;
            spec.flipped = false;
            spec.data = textureData.data() + (std::get<0>(tile)
                * _width * _height * sizePerElem * numChannels);
            image->Read(spec);
        }
    }

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0,
                 _internalFormat,
                 static_cast<GLsizei>(_width),
                 static_cast<GLsizei>(_height),
                 static_cast<GLsizei>(_depth),
                 0, _format, type, textureData.data());

    GLF_POST_PENDING_GL_ERRORS();

    _SetMemoryUsed(numBytes);
}

PXR_NAMESPACE_CLOSE_SCOPE
