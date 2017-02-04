/*
 * usdCacheFormat.h
 *
 *  Created on: Jan 19, 2017
 *      Author: orenouard
 */

#ifndef PXRUSDMAYA_CACHE_FORMAT_H
#define PXRUSDMAYA_CACHE_FORMAT_H

/// \file usdCacheFormat.h

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
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

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
        UsdCacheFormat();
        ~UsdCacheFormat() override;
        static void* creator();
        static char const* translatorName();
        MString extension() override;
        bool handlesDescription() override;

        MStatus open(const MString& fileName,
                FileAccessMode mode) override;
        void close() override;
        MStatus isValid() override;
        MStatus rewind() override;

        MStatus writeHeader(const MString& version,
                MTime& startTime,
                MTime& endTime) override;
        MStatus readHeader() override;

        MStatus writeTime(MTime& time) override;
        MStatus readTime(MTime& time) override;
        MStatus findTime(MTime& time, MTime& foundTime) override;
        MStatus readNextTime(MTime& foundTime) override;

        void beginWriteChunk() override;
        void endWriteChunk() override;
        MStatus beginReadChunk() override;
        void endReadChunk() override;

        MStatus writeInt32(int) override;
        int readInt32() override;
        unsigned readArraySize() override;
        MStatus writeDoubleArray(const MDoubleArray&) override;
        MStatus readDoubleArray(MDoubleArray&, unsigned size) override;
        MStatus writeFloatArray(const MFloatArray&) override;
        MStatus readFloatArray(MFloatArray&, unsigned size) override;
        MStatus writeDoubleVectorArray(const MVectorArray& array) override;
        MStatus readDoubleVectorArray(MVectorArray&, unsigned arraySize) override;
        MStatus writeFloatVectorArray(const MFloatVectorArray& array) override;
        MStatus readFloatVectorArray(MFloatVectorArray& array, unsigned arraySize) override;
        MStatus writeChannelName(const MString& name) override;
        MStatus findChannelName(const MString& name) override;
        MStatus readChannelName(MString& name) override;

    private:
        // Attributes correspondance and type do not follow a systematic rule
        // so we need some structure to store their specificities.
        // We will have to access sequentially,since order of creation is important
        // for Maya readChannelName, but also by Maya name or USD name for some calls
        struct attributeDefinition {
            std::string mayaAttrName = "";
            MCacheFormatDescription::CacheDataType mayaDataType = MCacheFormatDescription::kUnknownData;
            TfToken usdAttrName = TfToken();
            SdfValueTypeName usdAttrType = SdfValueTypeName();
            std::function<VtValue(VtValue x)> convertToUsd = nullptr;
            std::function<VtValue(VtValue x)> convertFromUsd = nullptr;

            attributeDefinition(const std::string& mayaAttrName = "",
                                MCacheFormatDescription::CacheDataType mayaDataType = MCacheFormatDescription::kUnknownData,
                                const std::string& usdAttrName = TfToken(),
                                const SdfValueTypeName& usdAttrType = SdfValueTypeName(),
                                const std::function<VtValue(VtValue x)> convertToUsd = nullptr,
                                const std::function<VtValue(VtValue x)> convertFromUsd = nullptr):
                mayaAttrName(mayaAttrName),
                mayaDataType(mayaDataType),
                usdAttrName(usdAttrName),
                usdAttrType(usdAttrType),
                convertToUsd(convertToUsd),
                convertFromUsd(convertFromUsd){}
        };
        struct maya{};
        struct usd{};
        using attributesSet = boost::multi_index::multi_index_container<
            attributeDefinition,
            boost::multi_index::indexed_by<
                boost::multi_index::sequenced<>,
                boost::multi_index::ordered_unique<
                    boost::multi_index::tag<maya>,
                    boost::multi_index::member<
                        attributeDefinition,
                        std::string,
                        &attributeDefinition::mayaAttrName
                    >
                >,
                boost::multi_index::ordered_unique<
                    boost::multi_index::tag<usd>,
                    boost::multi_index::member<
                        attributeDefinition,
                        TfToken,
                        &attributeDefinition::usdAttrName
                    >
                >
            >
        >;
        using attrById = attributesSet::nth_index<0>::type;
        // Internal methods for lookup and conversion
        SdfPath findOrAddDefaultPrim();
        SdfValueTypeName MCacheDataTypeToSdfValueTypeName(
            const MCacheFormatDescription::CacheDataType dataType);
        MCacheFormatDescription::CacheDataType  SdfValueTypeNameToMCacheDataType(
            const SdfValueTypeName& attrType);
        SdfPath usdPathFromAttribute(const TfToken& attr);
        attrById::iterator findOrAddMayaAttribute(const std::string& mayaAttrName,
                           const MCacheFormatDescription::CacheDataType mayaDataType);
        attrById::iterator findOrAddUsdAttribute(const TfToken& usdAttrName,
                                      const SdfValueTypeName& usdAttrType);
        MStatus readExistingAttributes();
        void writeMetadata();
        void closeLayer();
        // Declare a set of predefined attributes for the specific cases
        // ie usd 'ids' are maya's 'id', usd 'points' are maya's 'position' etc..
        static const attributesSet mDefinedAttributes;
        // State keeping during access
        SdfLayerRefPtr mLayerPtr;
        double mCurrentTime;
        attrById::iterator mFirstMayaAttr;
        // We still need to store the name and not just the iterator for deferred add during kWrite,
        // we could store both and iterator and a name to get a little bit of read speed
        std::string mCurrentChannel;
        // Dynamic set for keeping track of created attributes
        attributesSet mCachedAttributes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_CACHE_FORMAT_H
