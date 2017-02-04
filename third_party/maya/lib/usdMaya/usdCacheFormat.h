/*
 * usdCacheFormat.cpp
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
#include <maya/MIntArray.h>
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

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaCacheFormatTokens,
                         PXRUSDMAYA_CACHEFORMAT_TOKENS);

using namespace boost::multi_index;

class usdCacheFormat: public MPxCacheFormat {
    public:
        usdCacheFormat();
        ~usdCacheFormat();
        static void* creator();
        static MString translatorName();
        MString extension();
        bool handlesDescription();

        MStatus open(const MString& fileName,
                FileAccessMode mode);
        void close();
        MStatus isValid();
        MStatus rewind();

        MStatus writeHeader(const MString& version,
                MTime& startTime,
                MTime& endTime);
        MStatus readHeader();

        MStatus writeTime(MTime& time);
        MStatus readTime(MTime& time);
        MStatus findTime(MTime& time, MTime& foundTime);
        MStatus readNextTime(MTime& foundTime);

        void beginWriteChunk();
        void endWriteChunk();
        MStatus beginReadChunk();
        void endReadChunk();

        MStatus writeInt32(int);
        int readInt32();
        unsigned readArraySize();
        MStatus writeDoubleArray(const MDoubleArray&);
        MStatus readDoubleArray(MDoubleArray&, unsigned size);
        MStatus writeFloatArray(const MFloatArray&);
        MStatus readFloatArray(MFloatArray&, unsigned size);
        MStatus writeDoubleVectorArray(const MVectorArray& array);
        MStatus readDoubleVectorArray(MVectorArray&, unsigned arraySize);
        MStatus writeFloatVectorArray(const MFloatVectorArray& array);
        MStatus readFloatVectorArray(MFloatVectorArray& array, unsigned arraySize);
        MStatus writeChannelName(const MString& name);
        MStatus findChannelName(const MString& name);
        MStatus readChannelName(MString& name);

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
            std::function<VtValue(VtValue x)> convertToUsd = NULL;
            std::function<VtValue(VtValue x)> convertFromUsd = NULL;

            attributeDefinition(const std::string& mayaAttrName = "",
                                MCacheFormatDescription::CacheDataType mayaDataType = MCacheFormatDescription::kUnknownData,
                                const std::string& usdAttrName = TfToken(),
                                const SdfValueTypeName& usdAttrType = SdfValueTypeName(),
                                const std::function<VtValue(VtValue x)> convertToUsd = NULL,
                                const std::function<VtValue(VtValue x)> convertFromUsd = NULL):
                mayaAttrName(mayaAttrName),
                mayaDataType(mayaDataType),
                usdAttrName(usdAttrName),
                usdAttrType(usdAttrType),
                convertToUsd(convertToUsd),
                convertFromUsd(convertFromUsd){}
        };
        struct maya{};
        struct usd{};
        typedef multi_index_container<
            attributeDefinition,
            indexed_by<
                sequenced<>,
                ordered_unique<
                    tag<maya>,
                    member<
                        attributeDefinition,
                        std::string,
                        &attributeDefinition::mayaAttrName
                    >
                >,
                ordered_unique<
                    tag<usd>,
                    member<
                        attributeDefinition,
                        TfToken,
                        &attributeDefinition::usdAttrName
                    >
                >
            >
        > attributesSet;
        typedef attributesSet::nth_index<0>::type attrById;
        // Internal methods for lookup and conversion
        SdfPath findOrAddDefaultPrim();
        SdfValueTypeName MCacheDataTypeToSdfValueTypeName(
            const MCacheFormatDescription::CacheDataType dataType);
        MCacheFormatDescription::CacheDataType  SdfValueTypeNameToMCacheDataType(
            const SdfValueTypeName& attrType);
        std::string mayaNodeFromChannel(const std::string& channel);
        std::string mayaAttrFromChannel(const std::string& channel);
        SdfPath usdPathFromAttribute(const TfToken& attr);
        attrById::iterator findOrAddMayaAttribute(const std::string& mayaAttrName,
                           const MCacheFormatDescription::CacheDataType mayaDataType);
        attrById::iterator findOrAddUsdAttribute(const TfToken& usdAttrName,
                                      const SdfValueTypeName& usdAttrType);
        MStatus readExistingAttributes();
        template <class T1, typename T2>
        VtArray<T2> convertOutArray(const T1& array, std::function<VtValue(VtValue x)> convertFn);
        template <class T1, typename T2>
        VtArray<T2> convertOutVectorArray(const T1& array, std::function<VtValue(VtValue x)> convertFn);
        template <typename T1, class T2>
        void convertInArray(const VtValue& value, T2& array, std::function<VtValue(VtValue x)> convertFn);
        template <typename T1, class T2>
        void convertInVectorArray(const VtValue& value, T2& array, std::function<VtValue(VtValue x)> convertFn);
        void writeMetadata();
        void closeLayer();
        // Declare a set of predefined attributes for the specific cases
        // ie usd 'ids' are maya's 'id', usd 'points' are maya's 'position' etc..
        static const attributesSet mDefinedAttributes;
        // Other static constants
        static const std::string mTranslatorName;
        static const MTime::Unit mTimeUnit;
        static const std::string mDefaultPrimName;
        static const std::string mDefaultPrimType;
        // Dynamic set for keeping track of created attributes
        attributesSet mCachedAttributes;
        SdfLayerRefPtr mLayerPtr;
        double mCurrentTime;
        std::string mFirstMayaAttr;
        std::string mCurrentChannel;
};

#endif // PXRUSDMAYA_CACHE_FORMAT_H
