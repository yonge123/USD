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
#ifndef PXRUSDMAYA_CACHE_FORMAT_H
#define PXRUSDMAYA_CACHE_FORMAT_H

/// \file usdCacheFormat.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include <maya/MPxCacheFormat.h>
#include <maya/MCacheFormatDescription.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTime.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MVectorArray.h>

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"

#include <string>

#define PXRUSDMAYA_CACHEFORMAT_TOKENS \
    ((UsdFileExtensionDefault, "usd")) \
    ((UsdFileExtensionASCII, "usda")) \
    ((UsdFileExtensionCrate, "usdc")) \
    ((UsdFileFilter, "*.usd *.usda *.usdc"))

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaCacheFormatTokens,
                         PXRUSDMAYA_CACHEFORMAT_TOKENS);

class UsdCacheFormat: public MPxCacheFormat {
public:
    constexpr static auto translatorName = "usd";
    static void* creator() { return new UsdCacheFormat(); };

    PXRUSDMAYA_API
    UsdCacheFormat();
    PXRUSDMAYA_API
    ~UsdCacheFormat() override;

    UsdCacheFormat(const UsdCacheFormat& o) = delete;
    UsdCacheFormat& operator=(const UsdCacheFormat& o) = delete;

    PXRUSDMAYA_API
    MString extension() override;
    PXRUSDMAYA_API
    bool handlesDescription() override;

    PXRUSDMAYA_API
    MStatus open(const MString& fileName,
            FileAccessMode mode) override;
    PXRUSDMAYA_API
    void close() override;
    PXRUSDMAYA_API
    MStatus isValid() override;
    PXRUSDMAYA_API
    MStatus rewind() override;

    PXRUSDMAYA_API
    MStatus writeHeader(const MString& version,
            MTime& startTime,
            MTime& endTime) override;
    PXRUSDMAYA_API
    MStatus readHeader() override;

    PXRUSDMAYA_API
    MStatus writeTime(MTime& time) override;
    PXRUSDMAYA_API
    MStatus readTime(MTime& time) override;
    PXRUSDMAYA_API
    MStatus findTime(MTime& time, MTime& foundTime) override;
    PXRUSDMAYA_API
    MStatus readNextTime(MTime& foundTime) override;

    PXRUSDMAYA_API
    void beginWriteChunk() override;
    PXRUSDMAYA_API
    void endWriteChunk() override;
    PXRUSDMAYA_API
    MStatus beginReadChunk() override;
    PXRUSDMAYA_API
    void endReadChunk() override;

    PXRUSDMAYA_API
    MStatus writeInt32(int) override;
    PXRUSDMAYA_API
    int readInt32() override;
    PXRUSDMAYA_API
    unsigned readArraySize() override;
    PXRUSDMAYA_API
    MStatus writeDoubleArray(const MDoubleArray&) override;
    PXRUSDMAYA_API
    MStatus readDoubleArray(MDoubleArray&, unsigned size) override;
    PXRUSDMAYA_API
    MStatus writeFloatArray(const MFloatArray&) override;
    PXRUSDMAYA_API
    MStatus readFloatArray(MFloatArray&, unsigned size) override;
    PXRUSDMAYA_API
    MStatus writeDoubleVectorArray(const MVectorArray& array) override;
    PXRUSDMAYA_API
    MStatus readDoubleVectorArray(MVectorArray&, unsigned arraySize) override;
    PXRUSDMAYA_API
    MStatus writeFloatVectorArray(const MFloatVectorArray& array) override;
    PXRUSDMAYA_API
    MStatus readFloatVectorArray(MFloatVectorArray& array, unsigned arraySize) override;
    PXRUSDMAYA_API
    MStatus writeChannelName(const MString& name) override;
    PXRUSDMAYA_API
    MStatus findChannelName(const MString& name) override;
    PXRUSDMAYA_API
    MStatus readChannelName(MString& name) override;

private:
    // Internal methods for lookup and conversion
    PXRUSDMAYA_API
    MStatus readExistingAttributes();
    PXRUSDMAYA_API
    void closeLayer();

    // Using an impl to avoid exposing complex boost headers.
    struct Impl;
    std::unique_ptr<Impl> impl;

    // State keeping during access
    SdfLayerRefPtr mLayerPtr;

    // We still need to store the name and not just the iterator for deferred add during kWrite,
    // we could store both and iterator and a name to get a little bit of read speed
    std::string mCurrentChannel;
    double mCurrentTime;
    // Writing time to the cache file is bugged out. If a single file is used
    // only the write header is called. If there is a file per frame, write header called once
    // with 0-0, then write time for each frame. So if write time is called, we have to
    // overwrite start and end time code the first time, and expand the range at subsequent calls.
    bool mWriteTimeCalledOnce;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_CACHE_FORMAT_H
