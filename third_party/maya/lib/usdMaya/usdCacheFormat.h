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

#define PXRUSDMAYA_CACHEFORMAT_TOKENS \
    ((UsdFileExtensionDefault, "usd")) \
    ((UsdFileExtensionASCII, "usda")) \
    ((UsdFileExtensionCrate, "usdc")) \
    ((UsdFileFilter, "*.usd *.usda *.usdc"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaCacheFormatTokens,
        PXRUSDMAYA_CACHEFORMAT_TOKENS);

class usdCacheFormat: public MPxCacheFormat {
	public:
		usdCacheFormat();
		~usdCacheFormat();
		static void* creator();
		MString translatorName();
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

		MStatus writeDescription(const MCacheFormatDescription& description,
				const MString& descriptionFileLocation,
				const MString& baseFileName);
		MStatus readDescription(MCacheFormatDescription& description,
				const MString& descriptionFileLocation,
				const MString& baseFileName);

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
		SdfPath addChannel(const MString& channelName,
				const MString& interpretation,
				const MCacheFormatDescription::CacheDataType dataType);
		void readMetadata();
		void writeMetadata();
		void closeLayer();

		std::map<std::string, SdfPath> mPathMap;
		std::string mFileName;
		FileAccessMode mFileMode;
		static const MTime::Unit mTimeUnit = MTime::k6000FPS;
		SdfLayerRefPtr mLayerPtr;
		bool mDescript;
		double mCurrentTime;
		std::string mCurrentChannel;
		SdfPath mCurrentPath;
};



#endif // PXRUSDMAYA_CACHE_FORMAT_H
