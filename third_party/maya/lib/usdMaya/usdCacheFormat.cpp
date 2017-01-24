/*
 * usdCacheFormat.cpp
 *
 *  Created on: Jan 19, 2017
 *      Author: orenouard
 */

#include "usdMaya/usdCacheFormat.h"

#include <string>
#include <cstring>
#include <stack>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <stdlib.h>

#include <maya/MGlobal.h>
#include <maya/MTime.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MVectorArray.h>

using namespace std;

string XMLSTARTTAG(string x) {
	return ("<" + x + ">");
}
string XMLENDTAG(string x) {
	return ("</" + x + ">");
}

string cacheTag("awGeoCache");
string startTimeTag("startTime");
string endTimeTag("endTime");
string versionTag("version");
string timeTag("time");
string sizeTag("size");
string intTag("integer32");
string doubleArrayTag("doubleArray");
string floatArrayTag("floatArray");
string doubleVectorArrayTag("doubleVectorArray");
string floatVectorArrayTag("floatVectorArray");
string channelTag("channel");
string chunkTag("chunk");
string Autodesk_Cache_File("Autodesk_Cache_File");
string cacheType_Type("cacheType-Type");
string OneFilePerFrame("OneFilePerFrame");
string OneFile("OneFile");
string cacheType_Format("cacheType-Format");
string time_Range("time-Range");
string cacheTimePerFrame_TimePerFrame("cacheTimePerFrame-TimePerFrame");
string cacheVersion_Version("cacheVersion-Version");
string extra("extra");
string Channels("Channels");
string ChannelName("ChannelName");
string ChannelType("ChannelType");
string ChannelInterpretation("ChannelInterpretation");
string SamplingType("SamplingType");
string Double("Double");
string DoubleArray("DoubleArray");
string DoubleVectorArray("DoubleVectorArray");
string Int32Array("Int32Array");
string FloatArray("FloatArray");
string FloatVectorArray("FloatVectorArray");
string Regular("Regular");
string Irregular("Irregular");
string SamplingRate("SamplingRate");
string StartTime("StartTime");
string EndTime("EndTime");

MString fCacheFormatName("pxrUsdCacheFormat");

usdCacheFormat::usdCacheFormat() {
}

usdCacheFormat::~usdCacheFormat() {
	close();
}

MString usdCacheFormat::translatorName() {
	return MString(fCacheFormatName);								// For presentation in GUI
}

MString usdCacheFormat::extension() {
	return MString("mc");											// For files on disk
}

bool usdCacheFormat::handlesDescription() {
	return true;
}

void* usdCacheFormat::creator() {
	return new usdCacheFormat();
}

MStatus
usdCacheFormat::open(
		const MString& fileName,
		FileAccessMode mode) {
	bool rtn = true;
	assert((fileName.length() > 0));
	fFileName = fileName;

	if (mode == kWrite) {
		fFile.open(fFileName.asChar(), ios::out);
	} else if (mode == kReadWrite) {
		fFile.open(fFileName.asChar(), ios::app);
	} else {
		fFile.open(fFileName.asChar(), ios::in);
	}
	if (!fFile.is_open()) {
		rtn = false;
	} else {
		if (mode == kRead) {
			rtn = readHeader();
		}
	}
	return rtn ? MS::kSuccess : MS::kFailure;
}

MStatus
usdCacheFormat::isValid() {
	bool rtn = fFile.is_open();
	return rtn ? MS::kSuccess : MS::kFailure;
}

MStatus
usdCacheFormat::readHeader() {
	bool rtn = false;
	if (kWrite != fMode) {
		if (fFile.is_open()) {
			string tag;
			readXmlTag(tag);
			if (tag == XMLSTARTTAG(cacheTag)) {
				MStringArray value;
				readXmlTagValue(versionTag, value);
				readXmlTagValue(startTimeTag, value);
				readXmlTagValue(endTimeTag, value);
				readXmlTag(tag);  // Should be header close tag, check
				if (tag != XMLENDTAG(cacheTag)) {
					// TODO - handle this error case
				}
				rtn = true;
			}
		}
	}
	return rtn ? MS::kSuccess : MS::kFailure;
}

MStatus
usdCacheFormat::rewind() {
	if (fFile.is_open()) {
		close();
		if (!open(fFileName, kRead)) {
			return MS::kFailure;
		}
		return MS::kSuccess;
	} else {
		return MS::kFailure;
	}
}

void
usdCacheFormat::close() {
	if (fFile.is_open()) {
		fFile.close();
	}
}

MStatus
usdCacheFormat::writeInt32(int i) {
	writeXmlTagValue(intTag, i);
	return MS::kSuccess;
}

int
usdCacheFormat::readInt32() {
	MStringArray value;
	readXmlTagValue(intTag, value);
	return atoi(value[0].asChar());
}

MStatus
usdCacheFormat::writeHeader(
		const MString& version,
		MTime& startTime,
		MTime& endTime) {
	stringstream oss;
	startXmlBlock(cacheTag);
	string v(version.asChar());
	writeXmlTagValue(versionTag, v);
	oss << startTime;
	writeXmlTagValue(startTimeTag, oss.str());
	oss.str("");
	oss.clear();
	oss << endTime;
	writeXmlTagValue(endTimeTag, oss.str());
	endXmlBlock();
	return MS::kSuccess;
}

MStatus usdCacheFormat::readTime(MTime& time) {
	MStringArray timeValue;
	readXmlTagValue(timeTag, timeValue);
	time.setValue(strtod(timeValue[0].asChar(), NULL));
	return MS::kSuccess;
}

MStatus usdCacheFormat::writeTime(MTime& time) {
	stringstream oss;
	oss << time;
	writeXmlTagValue(timeTag, oss.str());
	return MS::kSuccess;
}

MStatus usdCacheFormat::findChannelName(const MString& name)
//  
//  Given that the right time has already been found, find the name
//  of the channel we're trying to read.
//
		{
	MStringArray value;
	while (readXmlTagValueInChunk(channelTag, value)) {
		if (value.length() == 1 && value[0] == name) {
			return MS::kSuccess;
		}
	}
	return MS::kFailure;
}

MStatus usdCacheFormat::readChannelName(MString& name)
//  
//  Given that the right time has already been found, find the name
//  of the channel we're trying to read.
//
//  If no more channels exist, return false. Some callers rely on this false return 
//  value to terminate scanning for channels, thus it's not an error condition.
//
//  TODO: the current implementation assumes there are numChannels of data stored
//  for every time.  This assumption needs to be removed.
//
		{
	MStringArray value;
	readXmlTagValueInChunk(channelTag, value);
	name = value[0];
	return name.length() == 0 ? MS::kFailure : MS::kSuccess;
}

MStatus usdCacheFormat::readNextTime(MTime& foundTime)
//  
// Read the next time based on the current read position.
//
		{
	MTime readAwTime(0.0, MTime::k6000FPS);
	bool ret = readTime(readAwTime);
	foundTime = readAwTime;
	return ret ? MS::kSuccess : MS::kFailure;
}

MStatus usdCacheFormat::findTime(MTime& time, MTime& foundTime)
//  
// Find the biggest cached time, which is smaller or equal to 
// seekTime and return foundTime
//
		{
	MTime timeTolerance(0.0, MTime::k6000FPS);
	MTime seekTime(time);
	MTime preTime(seekTime - timeTolerance);
	MTime postTime(seekTime + timeTolerance);
	bool fileRewound = false;
	while (1) {
		bool timeTagFound = beginReadChunk();
		if (!timeTagFound && !fileRewound) {
			if (!rewind()) {
				return MS::kFailure;
			}
			fileRewound = true;
			timeTagFound = beginReadChunk();
		}
		if (timeTagFound) {
			MTime rTime(0.0, MTime::k6000FPS);
			readTime(rTime);
			if (rTime >= preTime && rTime <= postTime) {
				foundTime = rTime;
				return MS::kSuccess;
			}
			if (rTime > postTime) {
				if (!fileRewound) {
					if (!rewind()) {
						return MS::kFailure;
					}
					fileRewound = true;
				} else {
					// Time could not be found
					//
					return MS::kFailure;
				}
			} else {
				fileRewound = true;
			}
			endReadChunk();
		} else {
			// Not a valid disk cache file.
			break;
		}
	}
	return MS::kFailure;
}

MStatus usdCacheFormat::writeChannelName(const MString& name) {
	string chan = name.asChar();
	writeXmlTagValue(channelTag, chan);
	return MS::kSuccess;
}

void usdCacheFormat::beginWriteChunk() {
	startXmlBlock(chunkTag);
}

void usdCacheFormat::endWriteChunk() {
	endXmlBlock();
}

MStatus usdCacheFormat::beginReadChunk() {
	return findXmlStartTag(chunkTag) ? MS::kSuccess : MS::kFailure;
}

void usdCacheFormat::endReadChunk() {
	findXmlEndTag(chunkTag);
}

MStatus usdCacheFormat::writeDoubleArray(const MDoubleArray& array) {
	int size = array.length();
	assert(size != 0);
	writeXmlTagValue(sizeTag, size);
	startXmlBlock(doubleArrayTag);
	for (int i = 0; i < size; i++) {
		writeXmlValue(array[i]);
	}
	endXmlBlock();
	return MS::kSuccess;
}

MStatus usdCacheFormat::writeFloatArray(const MFloatArray& array) {
	int size = array.length();
	assert(size != 0);
	writeXmlTagValue(sizeTag, size);
	startXmlBlock(floatArrayTag);
	for (int i = 0; i < size; i++) {
		writeXmlValue(array[i]);
	}
	endXmlBlock();
	return MS::kSuccess;
}

MStatus usdCacheFormat::writeDoubleVectorArray(const MVectorArray& array) {
	int size = array.length();
	assert(size != 0);
	writeXmlTagValue(sizeTag, size);
	startXmlBlock(doubleVectorArrayTag);
	for (int i = 0; i < size; i++) {
		writeXmlValue(array[i][0]);
		writeXmlValue(array[i][1]);
		writeXmlValue(array[i][2]);
		string endl("\n");
		writeXmlValue(endl);
	}
	endXmlBlock();
	return MS::kSuccess;
}

MStatus usdCacheFormat::writeFloatVectorArray(const MFloatVectorArray& array) {
	int size = array.length();
	assert(size != 0);
	writeXmlTagValue(sizeTag, size);
	startXmlBlock(floatVectorArrayTag);
	for (int i = 0; i < size; i++) {
		writeXmlValue(array[i][0]);
		writeXmlValue(array[i][1]);
		writeXmlValue(array[i][2]);
		string endl("\n");
		writeXmlValue(endl);
	}
	endXmlBlock();
	return MS::kSuccess;
}

unsigned usdCacheFormat::readArraySize() {
	MStringArray value;
	if (readXmlTagValue(sizeTag, value)) {
		unsigned readSize = (unsigned) atoi(value[0].asChar());
		return readSize;
	}
	return 0;
}

MStatus usdCacheFormat::readDoubleArray(MDoubleArray& array,
		unsigned arraySize) {
	MStringArray value;
	readXmlTagValue(doubleArrayTag, value);
	assert(value.length() == arraySize);
	array.setLength(arraySize);
	for (unsigned int i = 0; i < value.length(); i++) {
		array[i] = strtod(value[i].asChar(), NULL);
	}

	return MS::kSuccess;
}

MStatus usdCacheFormat::readFloatArray(MFloatArray& array, unsigned arraySize) {
	MStringArray value;
	readXmlTagValue(floatArrayTag, value);
	assert(value.length() == arraySize);
	array.setLength(arraySize);
	for (unsigned int i = 0; i < value.length(); i++) {
		array[i] = (float) strtod(value[i].asChar(), NULL);
	}

	return MS::kSuccess;
}

MStatus usdCacheFormat::readDoubleVectorArray(MVectorArray& array,
		unsigned arraySize) {
	MStringArray value;
	if (!readXmlTagValue(doubleVectorArrayTag, value)) {
		return MS::kFailure;
	}
	assert(value.length() == arraySize * 3);
	array.setLength(arraySize);
	for (unsigned i = 0; i < arraySize; i++) {
		double v[3];
		v[0] = strtod(value[i * 3].asChar(), NULL);
		v[1] = strtod(value[i * 3 + 1].asChar(), NULL);
		v[2] = strtod(value[i * 3 + 2].asChar(), NULL);
		array.set(v, i);
	}

	return MS::kSuccess;
}

MStatus usdCacheFormat::readFloatVectorArray(MFloatVectorArray& array,
		unsigned arraySize) {
	MStringArray value;
	readXmlTagValue(floatVectorArrayTag, value);
	assert(value.length() == arraySize * 3);
	array.clear();
	array.setLength(arraySize);
	for (unsigned i = 0; i < arraySize; i++) {
		float v[3];
		v[0] = (float) strtod(value[i * 3].asChar(), NULL);
		v[1] = (float) strtod(value[i * 3 + 1].asChar(), NULL);
		v[2] = (float) strtod(value[i * 3 + 2].asChar(), NULL);
		array.set(v, i);
	}
	return MS::kSuccess;
}

static bool readKeyAndValue(fstream& file, std::string& key,
		std::string& value) {
	char line[256];
	for (;;) {
		if (file.eof()) {
			key.clear();
			value.clear();
			return true;
		}
		if (!file.good())
			return false;
		line[0] = 0;
		file.getline(line, sizeof line);
		if (line[0] != 0)
			break;
	}
	char* delimiter = std::strchr(line, '=');
	if (delimiter == NULL)
		return false;
	*delimiter = 0;
	key = line;
	value = delimiter + 1;
	return true;
}

static bool readValue(fstream& file, const std::string& key,
		std::string& value) {
	std::string readKey;
	if (!readKeyAndValue(file, readKey, value))
		return false;
	if (readKey != key)
		return false;
	return true;
}

static bool readValue(fstream& file, const std::string& key, MTime& time) {
	std::string value;
	if (!readValue(file, key, value))
		return false;
	double d;
	if (1 != sscanf(value.c_str(), "%lf", &d))
		return false;
	time = MTime(d, MTime::k6000FPS);
	return true;
}

MStatus usdCacheFormat::readDescription(MCacheFormatDescription& description,
		const MString& descriptionFileLocation, const MString& baseFileName) {
	const MString filename = descriptionFileLocation + baseFileName + ".txt";
	fstream file;
	file.open(filename.asChar(), ios::in);
	if (!file.is_open())
		return MS::kFailure;
	char line[256] = "";
	file.getline(line, sizeof line);
	if (line != Autodesk_Cache_File)
		return MS::kFailure;
	MStatus status = MS::kSuccess;
	int channels = 0;
	while (file.good()) {
		std::string key, value;
		if (!readKeyAndValue(file, key, value)) {
			status = MS::kFailure;
			break;
		}
		if (key == cacheType_Type) {
			MCacheFormatDescription::CacheFileDistribution distribution =
					MCacheFormatDescription::kNoFile;
			if (value == OneFilePerFrame)
				distribution = MCacheFormatDescription::kOneFilePerFrame;
			else if (value == OneFile)
				distribution = MCacheFormatDescription::kOneFile;
			else {
				status = MS::kFailure;
				break;
			}
			description.setDistribution(distribution);
		} else if (key == cacheType_Format) {
			if (value != fCacheFormatName.asChar()) {
				status = MS::kFailure;
				break;
			}
		} else if (key == time_Range) {
			// no-op
		} else if (key == cacheTimePerFrame_TimePerFrame) {
			double time = 0.0;
			sscanf(value.c_str(), "%lf", &time);
			if (time == 0.0) {
				status = MS::kFailure;
				break;
			}
			description.setTimePerFrame(MTime(time, MTime::k6000FPS));
		} else if (key == cacheVersion_Version) {
			// no-op
		} else if (key == extra) {
			description.addDescriptionInfo(MString(value.c_str()));
		} else if (key == Channels) {
			sscanf(value.c_str(), "%d", &channels);
			break;
		}
	}
	if (status && !channels) {
		status = MS::kFailure;
	}
	if (status) {
		// parse channels
		while (file.good()) {
			std::string key, channelName;
			if (!readKeyAndValue(file, key, channelName)) {
				status = MS::kFailure;
				break;
			}
			if (key != ChannelName)
				continue;
			std::string interpretation, dataTypeString, samplingTypeString;
			if (!readValue(file, ChannelType, dataTypeString)) {
				status = MS::kFailure;
				break;
			}
			if (!readValue(file, ChannelInterpretation, interpretation)) {
				status = MS::kFailure;
				break;
			}
			if (!readValue(file, SamplingType, samplingTypeString)) {
				status = MS::kFailure;
				break;
			}
			MCacheFormatDescription::CacheDataType dataType;
			if (dataTypeString == Double) {
				dataType = MCacheFormatDescription::kDouble;
			} else if (dataTypeString == DoubleArray) {
				dataType = MCacheFormatDescription::kDoubleArray;
			} else if (dataTypeString == DoubleVectorArray) {
				dataType = MCacheFormatDescription::kDoubleVectorArray;
			} else if (dataTypeString == Int32Array) {
				dataType = MCacheFormatDescription::kInt32Array;
			} else if (dataTypeString == FloatArray) {
				dataType = MCacheFormatDescription::kFloatArray;
			} else if (dataTypeString == FloatVectorArray) {
				dataType = MCacheFormatDescription::kFloatVectorArray;
			} else {
				status = MS::kFailure;
				break;
			}
			MCacheFormatDescription::CacheSamplingType samplingType;
			if (samplingTypeString == Regular) {
				samplingType = MCacheFormatDescription::kRegular;
			} else if (samplingTypeString == Irregular) {
				samplingType = MCacheFormatDescription::kIrregular;
			} else {
				status = MS::kFailure;
				break;
			}
			MTime samplingRate, startTime, endTime;
			if (!readValue(file, SamplingRate, samplingRate)) {
				status = MS::kFailure;
				break;
			}
			if (!readValue(file, StartTime, startTime)) {
				status = MS::kFailure;
				break;
			}
			if (!readValue(file, EndTime, endTime)) {
				status = MS::kFailure;
				break;
			}
			description.addChannel(MString(channelName.c_str()),
					MString(interpretation.c_str()), dataType, samplingType,
					samplingRate, startTime, endTime, &status);
			if (!status)
				break;
		}
	}
	file.close();
	return status;
}

MStatus usdCacheFormat::writeDescription(
		const MCacheFormatDescription& description,
		const MString& descriptionFileLocation, const MString& baseFileName) {
	const MString filename = descriptionFileLocation + baseFileName + ".txt";
	fstream file;
	file.open(filename.asChar(), ios::out);
	if (!file.is_open())
		return MS::kFailure;
	MStatus status = MS::kSuccess;
	file << Autodesk_Cache_File << "\n";
	file << cacheType_Type << "=";
	MCacheFormatDescription::CacheFileDistribution distribution =
			description.getDistribution();
	switch (distribution) {
	case MCacheFormatDescription::kOneFile:
		file << OneFile;
		break;
	case MCacheFormatDescription::kOneFilePerFrame:
		file << OneFilePerFrame;
		break;
	default:
		status = MS::kFailure;
		break;
	}
	file << "\n";
	file << cacheType_Format << "=" << fCacheFormatName << "\n";
	MTime startTime, endTime;
	description.getStartAndEndTimes(startTime, endTime);
	file << time_Range << "=" << (int) startTime.as(MTime::k6000FPS) << "-"
			<< (int) endTime.as(MTime::k6000FPS) << "\n";
	MTime timePerFrame = description.getTimePerFrame();
	file << cacheTimePerFrame_TimePerFrame << "="
			<< (int) timePerFrame.as(MTime::k6000FPS) << "\n";
	file << cacheVersion_Version << "=" << "2.0" << "\n";
	MStringArray info;
	description.getDescriptionInfo(info);
	for (unsigned i = 0; i < info.length(); ++i)
		file << extra << "=" << info[i] << "\n";
	unsigned int channels = description.getNumChannels();
	file << Channels << "=" << channels << "\n";
	for (unsigned i = 0; i < channels; ++i) {
		MString channelName = description.getChannelName(i);
		MString interpretation = description.getChannelInterpretation(i);
		MCacheFormatDescription::CacheDataType dataType =
				description.getChannelDataType(i);
		MCacheFormatDescription::CacheSamplingType samplingType =
				description.getChannelSamplingType(i);
		MTime samplingRate = description.getChannelSamplingRate(i);
		MTime startTime = description.getChannelStartTime(i);
		MTime endTime = description.getChannelEndTime(i);
		file << ChannelName << "=" << channelName << "\n";
		file << ChannelType << "=";
		switch (dataType) {
		case MCacheFormatDescription::kDouble:
			file << Double;
			break;
		case MCacheFormatDescription::kDoubleArray:
			file << DoubleArray;
			break;
		case MCacheFormatDescription::kDoubleVectorArray:
			file << DoubleVectorArray;
			break;
		case MCacheFormatDescription::kInt32Array:
			file << Int32Array;
			break;
		case MCacheFormatDescription::kFloatArray:
			file << FloatArray;
			break;
		case MCacheFormatDescription::kFloatVectorArray:
			file << FloatVectorArray;
			break;
		default:
			status = MS::kFailure;
			break;
		}
		file << "\n";
		file << ChannelInterpretation << "=" << interpretation << "\n";
		file << SamplingType << "=";
		switch (samplingType) {
		case MCacheFormatDescription::kRegular:
			file << Regular;
			break;
		case MCacheFormatDescription::kIrregular:
			file << Irregular;
			break;
		default:
			status = MS::kFailure;
			break;
		}
		file << "\n";
		file << SamplingRate << "=" << (int) samplingRate.as(MTime::k6000FPS)
				<< "\n";
		file << StartTime << "=" << (int) startTime.as(MTime::k6000FPS) << "\n";
		file << EndTime << "=" << (int) endTime.as(MTime::k6000FPS) << "\n";
	}
	file.close();
	return status;
}

// ****************************************
//
//  Helper functions
//

void usdCacheFormat::startXmlBlock(string& t) {
	fXmlStack.push(t);
	fFile << "<" << t << ">\n";
}

void usdCacheFormat::endXmlBlock() {
	string block = fXmlStack.top();
	fFile << "</" << block << ">\n";
	fXmlStack.pop();
}

void usdCacheFormat::writeXmlTagValue(string& tag, string value) {
	for (unsigned int i = 0; i < fXmlStack.size(); i++)
		fFile << "\t";
	fFile << "<" << tag << "> ";    // Extra space is important for reading
	fFile << value;
	fFile << " </" << tag << ">\n"; // Extra space is important for reading
}
void usdCacheFormat::writeXmlTagValue(string& tag, int value) {
	for (unsigned int i = 0; i < fXmlStack.size(); i++)
		fFile << "\t";
	fFile << "<" << tag << "> ";    // Extra space is important for reading
	ostringstream oss;
	oss << value;
	fFile << oss.str();
	fFile << " </" << tag << ">\n"; // Extra space is important for reading
}

bool usdCacheFormat::readXmlTagValue(string tag, MStringArray& values) {
	string endTag = XMLENDTAG(tag);
	bool status = true;
	values.clear();
	// Yes this could be much much smarter
	if (findXmlStartTag(tag)) {
		string token;
		fFile >> token;
		while (!fFile.eof() && token != endTag) {
			values.append(token.data());
			fFile >> token;
		}
	} else {
		status = false;
	}
	return status;
}

bool usdCacheFormat::readXmlTagValueInChunk(string tag, MStringArray& values) {
	string endTag = XMLENDTAG(tag);
	bool status = true;
	values.clear();
	// Find the tag in the currently read chunk.
	if (findXmlStartTagInChunk(tag)) {
		string token;
		fFile >> token;
		// Look for the values within the bounds of the tag.
		while (!fFile.eof() && token != endTag) {
			values.append(token.data());
			fFile >> token;
		}
	} else {
		status = false;
	}
	return status;
}

void usdCacheFormat::readXmlTag(string& value) {
	value.clear();
	fFile >> value;
}

bool usdCacheFormat::findXmlStartTag(string& tag) {
	string tagRead;
	string tagExpected = XMLSTARTTAG(tag);
	fFile >> tagRead;
	// Keep looking all the way to EOF
	while (!fFile.eof() && tagRead != tagExpected) {
		fFile >> tagRead;
	}
	if (tagRead != tagExpected) {
	}
	return (tagRead == tagExpected);
}

bool usdCacheFormat::findXmlStartTagInChunk(string& tag)
//
// Look for the given tag within the currently read chunk.
//
		{
	string tagRead;
	string tagExpected = XMLSTARTTAG(tag);
	string tagEndChunk("</" + chunkTag + ">");
	fFile >> tagRead;
	// Keep looking all the way to EOF
	while ((!fFile.eof()) && (tagRead != tagExpected)
			&& (tagRead != tagEndChunk)) {
		fFile >> tagRead;
	}
	if ((tagRead != tagExpected) && (tagRead != tagEndChunk)) {
	}
	return (tagRead == tagExpected);
}

bool usdCacheFormat::findXmlEndTag(string& tag) {
	string tagRead;
	string tagExpected("</" + tag + ">");
	fFile >> tagRead;
	if (tagRead != tagExpected) {
	}
	return (tagRead == tagExpected);
}

void usdCacheFormat::writeXmlValue(string& value) {
	fFile << value << " ";
}

void usdCacheFormat::writeXmlValue(double value) {
	fFile << value << " ";
}

void usdCacheFormat::writeXmlValue(float value) {
	fFile << value << " ";
}

void usdCacheFormat::writeXmlValue(int value) {
	fFile << value << " ";
}
