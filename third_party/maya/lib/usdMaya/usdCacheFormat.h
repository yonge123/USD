/*
 * usdCacheFormat.cpp
 *
 *  Created on: Jan 19, 2017
 *      Author: orenouard
 */

#ifndef PXRUSDMAYA_CACHE_FORMAT_H
#define PXRUSDMAYA_CACHE_FORMAT_H

/// \file usdCacheFormat.h

#include <maya/MFileObject.h>
#include <maya/MPxCacheFormat.h>
#include <maya/MCacheFormatDescription.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>
#include <stack>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <stdlib.h>

// #include <maya/MGlobal.h>
#include <maya/MTime.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MVectorArray.h>

using namespace std;

class usdCacheFormat: public MPxCacheFormat {
	public:
		usdCacheFormat();
		~usdCacheFormat();
		static void* creator();
		MString translatorName();
		MString extension();

		bool handlesDescription();
		MStatus readDescription(MCacheFormatDescription& description,
				const MString& descriptionFileLocation,
				const MString& baseFileName);
		MStatus writeDescription(const MCacheFormatDescription& description,
				const MString& descriptionFileLocation,
				const MString& baseFileName);


		MStatus isValid();
		MStatus open(const MString& fileName, FileAccessMode mode);
		void close();
		MStatus readHeader();
		MStatus writeHeader(const MString& version, MTime& startTime,
				MTime& endTime);
		void beginWriteChunk();
		void endWriteChunk();
		MStatus beginReadChunk();
		void endReadChunk();
		MStatus writeTime(MTime& time);
		MStatus readTime(MTime& time);
		MStatus findTime(MTime& time, MTime& foundTime);
		MStatus readNextTime(MTime& foundTime);
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
		MStatus writeInt32(int);
		int readInt32();
		MStatus rewind();

	protected:
		static MString comment(const MString& text);
		static MString quote(const MString& text);

	private:
		void startXmlBlock(string& t);
		void endXmlBlock();
		void writeXmlTagValue(string& tag, string value);
		void writeXmlTagValue(string& tag, int value);
		bool readXmlTagValue(string tag, MStringArray& value);
		bool readXmlTagValueInChunk(string tag, MStringArray& values);
		void readXmlTag(string& value);
		bool findXmlStartTag(string& tag);
		bool findXmlStartTagInChunk(string& tag);
		bool findXmlEndTag(string& tag);
		void writeXmlValue(string& value);
		void writeXmlValue(double value);
		void writeXmlValue(float value);
		void writeXmlValue(int value);

	MString fFileName;
	fstream fFile;
	stack<string> fXmlStack;
	FileAccessMode fMode;
};


#endif // PXRUSDMAYA_CACHE_FORMAT_H
