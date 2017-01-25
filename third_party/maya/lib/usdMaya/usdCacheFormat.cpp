/*
 * usdCacheFormat.cpp
 *
 *  Created on: Jan 19, 2017
 *      Author: orenouard
 */

#include <maya/MGlobal.h>

#include "usdMaya/usdCacheFormat.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"

TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaCacheFormatTokens,
		PXRUSDMAYA_CACHEFORMAT_TOKENS);

usdCacheFormat::usdCacheFormat() {
}

usdCacheFormat::~usdCacheFormat() {
}

MString usdCacheFormat::translatorName() {
	// For presentation in GUI
	return MString("pxrUsdCacheFormat");
}

MString usdCacheFormat::extension() {
	// For filtering display of files on disk
	return MString(
	        PxrUsdMayaCacheFormatTokens->UsdFileExtensionDefault.GetText());
}

bool usdCacheFormat::handlesDescription() {
	return true;
}

void* usdCacheFormat::creator() {
	return new usdCacheFormat();
}

MStatus usdCacheFormat::open(const MString& fileName, FileAccessMode mode) {
	if (fileName.length() == 0) {
		MGlobal::displayError("usdCacheFormat::open: empty filename");
		return MS::kFailure;
	}
	std::string iFileName = fileName.asChar();
	std::string iFileBase = TfStringGetBeforeSuffix(iFileName, '.');
	std::string iFileExtension = TfStringGetSuffix(iFileName, '.');
	const std::string usdExt = PxrUsdMayaCacheFormatTokens->UsdFileExtensionDefault.GetText();

	// Can kReadWrite happen and what behavior is it?
	if (mode == kWrite) {
		// If writing, make sure the file name is a valid one with a proper USD extension.
		if (iFileExtension != usdExt) {
			iFileName = TfStringPrintf("%s.%s", iFileName.c_str(), usdExt.c_str());
		}
		// Would we need to use CreateIdentifier?
		if (mLayerPtr) {
			if ((mLayerPtr->GetIdentifier() != iFileName)
					|| (not this->isValid())) {
				this->closeLayer();
			}
		}
		if (not mLayerPtr) {
			// If we are overwriting a file that already has been loaded
			// should we trash the layer or edit it?
			mLayerPtr = SdfLayer::Find(iFileName);
			if (not this->isValid()) {
				this->closeLayer();
			}
		}
		if (not mLayerPtr) {
			mLayerPtr = SdfLayer::CreateNew(iFileName);
			mDescript = false;
		}
		if (mLayerPtr) {
			mLayerPtr->SetPermissionToEdit(true);
			mLayerPtr->SetPermissionToSave(true);
			mFileName = iFileName;
			mFileMode = mode;
			return MS::kSuccess;
		} else {
			MGlobal::displayError(
			        "usdCacheFormat::open: (write) could not create a layer to "
			                + MString(iFileName.c_str()));
			return MS::kFailure;
		}
	} else if (mode == kRead) {
		// If no extension was passed we add the default usd one (we keep existing ones
		// in case you want to explicitely load other supported files?)
		// Since Maya expects a separate description frame it will recall open taking
		// upon itself to add .ext to the filename even if extension is already present.
		if (iFileExtension == "") {
			iFileExtension == usdExt;
			iFileName = TfStringPrintf("%s.%s", iFileName.c_str(), usdExt.c_str());
		} else if ((iFileExtension == usdExt) && (TfStringGetSuffix(iFileBase, '.') == usdExt)) {
			iFileName = iFileBase;
		}
		if (mLayerPtr) {
			if (mLayerPtr->GetIdentifier() != iFileName) {
				this->closeLayer();
			}
		}
		if (not mLayerPtr) {
			mLayerPtr = SdfLayer::FindOrOpen(iFileName);
		}
		if (mLayerPtr) {
			// Success, probably pointless to store opened file name
			if (this->isValid()) {
				mLayerPtr->SetPermissionToEdit(false);
				mLayerPtr->SetPermissionToSave(false);
				mFileName = iFileName;
				mFileMode = mode;
				return MS::kSuccess;
			} else {
				MGlobal::displayError("usdCacheFormat::open: (read) invalid cache format "
					+ MString(iFileName.c_str()));
				return MS::kFailure;
			}
		} else {
			MGlobal::displayError("usdCacheFormat::open: (read) could not read a layer from "
				+ MString(iFileName.c_str()));
			return MS::kFailure;
		}
	} else {
		MGlobal::displayError(
		        "usdCacheFormat::open: append (read + write) file open mode unsupported");
		return MS::kFailure;
	}
}

void usdCacheFormat::closeLayer()
{
	if (mLayerPtr) {
		mPathMap.clear();
		mDescript = false;
		mLayerPtr.Reset();
	}
}

void usdCacheFormat::close()
{
	// Maya can call close file directly but we need to control when we dump a layer if ever
}

MStatus usdCacheFormat::isValid() {
	// Check that the actual structures are actually there
	if (mLayerPtr) {
		return this->readHeader();
	} else {
		return MS::kFailure;
	}
}

MStatus usdCacheFormat::writeHeader(const MString& version,
		MTime& startTime,
		MTime& endTime) {
	if (mLayerPtr) {
		mLayerPtr->SetComment(std::string(this->translatorName().asChar())
			+ std::string(" version ") + version.asChar());
		mLayerPtr->SetStartTimeCode(startTime.asUnits(mTimeUnit));
		mLayerPtr->SetEndTimeCode(endTime.asUnits(mTimeUnit));
		return MS::kSuccess;
	} else {
		return MS::kFailure;
	}
}

MStatus usdCacheFormat::readHeader() {
	if (mLayerPtr) {
		const std::string comment = mLayerPtr->GetComment();
		const std::string expected = this->translatorName().asChar();
		if (TfStringStartsWith(comment, expected)) {
			return MS::kSuccess;
		}
	}
	return MS::kFailure;
}

MStatus usdCacheFormat::rewind() {
	if (mLayerPtr) {
		// Do we want to refresh if layer changed on disk?
		// mLayerPtr->Reload();
		// Should we even do anything ?
		if (mLayerPtr->HasStartTimeCode()) {
			mCurrentTime = mLayerPtr->GetStartTimeCode();
			return MS::kSuccess;
		} else {
			return MS::kFailure;
		}
	} else {
		return MS::kFailure;
	}
}

SdfPrimSpecHandle usdCacheFormat::addPrim(const SdfPath& specPath,
                                          const SdfSpecifier spec,
                                          const std::string& typeName) {
	if (specPath.IsPrimPath()) {
		SdfPrimSpecHandle parent = this->addPrim(specPath.GetParentPath(), spec, typeName);
		if (parent) {
			return SdfPrimSpec::New(parent, specPath.GetName(), spec, typeName);
		} else {
			return SdfPrimSpec::New(mLayerPtr, specPath.GetName(), spec, typeName);
		}
	} else {
		return SdfPrimSpecHandle();
	}
}

SdfPath usdCacheFormat::addChannel(const MString& channelName,
		const MString& interpretation,
		const MCacheFormatDescription::CacheDataType dataType) {
	const std::string channel(channelName.asChar());
	std::string mayaDag;
	const std::string attrName = interpretation.asChar();
	const std::size_t attrPos = channel.rfind(attrName);
	if (attrPos != std::string::npos) {
		mayaDag = channel.substr(0, attrPos - 1);
	} else {
		MGlobal::displayError(
				"usdCacheFormat::writeDescription: Can't extract a maya dag path from channel name "
						+ channelName
						+ " and interpretation name "
						+ interpretation);
		return SdfPath();
	}
	std::string cachePrimPathStr = mayaDag;
	cachePrimPathStr = TfStringReplace(cachePrimPathStr, "|", "/");
	cachePrimPathStr = TfStringReplace(cachePrimPathStr, ":", "_");
	if (!SdfPath::IsValidPathString(cachePrimPathStr)) {
		MGlobal::displayError(
				"usdCacheFormat::writeDescription: Can't make a valid prim spec path from "
						+ MString(mayaDag.c_str()));
		return SdfPath();
	}

	SdfPath specPath = SdfPath(cachePrimPathStr).GetPrimPath();
	SdfPrimSpecHandle geomPointsSpec = mLayerPtr->GetPrimAtPath(specPath);
	if (not geomPointsSpec) {
		// No SdfPrimSpec declared yet we need to declare it
		// Need an absolute path
		specPath = specPath.MakeAbsolutePath(SdfPath::AbsoluteRootPath());
		geomPointsSpec = this->addPrim(specPath, SdfSpecifierOver, "");
	}
	if (not geomPointsSpec) {
		MGlobal::displayError(
				"usdCacheFormat::writeDescription: Could not create a specification to represent "
						+ MString(specPath.GetString().c_str()));
		return SdfPath();
	}
	if (!SdfAttributeSpec::IsValidName(attrName)) {
		MGlobal::displayError(
				"usdCacheFormat::writeDescription: Invalid attribute name "
						+ MString(attrName.c_str()));
		return SdfPath();
	}
	SdfPath ownerPath = geomPointsSpec->GetPath();
	SdfPath attrPath = ownerPath.AppendProperty(TfToken(attrName));
	if (not mLayerPtr->GetAttributeAtPath(attrPath)) {
		SdfValueTypeName attrType;
		switch (dataType) {
		case MCacheFormatDescription::kDouble:
			attrType = SdfValueTypeNames->Double;
			break;
		case MCacheFormatDescription::kDoubleArray:
			attrType = SdfValueTypeNames->DoubleArray;
			break;
		case MCacheFormatDescription::kInt32Array:
			attrType = SdfValueTypeNames->IntArray;
			break;
		case MCacheFormatDescription::kFloatArray:
			attrType = SdfValueTypeNames->FloatArray;
			break;
		case MCacheFormatDescription::kDoubleVectorArray:
			attrType = SdfValueTypeNames->Vector3dArray;
			break;
		case MCacheFormatDescription::kFloatVectorArray:
			attrType = SdfValueTypeNames->Vector3fArray;
			break;
		default:
			MGlobal::displayError(
					"usdCacheFormat::writeDescription: Unknown attribute type for "
							+ MString(attrName.c_str()));
			return SdfPath();
		}
		// I can get null returns though attribute is there when checking exported file?
		SdfAttributeSpecHandle attrSpec = SdfAttributeSpec::New(geomPointsSpec,
				attrName, attrType,
				SdfVariability::SdfVariabilityVarying,
				false);
	}
	if (mLayerPtr->GetAttributeAtPath(attrPath)) {
		// Cache it
		mPathMap[channelName.asChar()] = attrPath;
		return attrPath;
	} else {
		MGlobal::displayError("usdCacheFormat::writeDescription: Failed to add "
				+ MString(attrPath.GetString().c_str()));
		return SdfPath();
	}
}

void usdCacheFormat::writeMetadata()
{
	VtDictionary pathMapMeta;
	for (auto& it : mPathMap) {
		pathMapMeta[it.first.c_str()] = it.second.GetString();
	}
	mLayerPtr->SetCustomLayerData(pathMapMeta);
}

void usdCacheFormat::readMetadata()
{
	VtDictionary pathMapMeta = mLayerPtr->GetCustomLayerData();
	mPathMap.clear();
	for (auto& it : pathMapMeta) {
		const std::string pathStr = it.second.Get<std::string>();
		mPathMap[it.first] = SdfPath(pathStr);
	}
}

MStatus usdCacheFormat::writeDescription(
        const MCacheFormatDescription& description,
        const MString& descriptionFileLocation, const MString& baseFileName) {
	// Maya calls it each frame after it does its writes, we need it done before first write
	if (mDescript) {
		return MStatus::kSuccess;
	}
	if (description.getDistribution() != MCacheFormatDescription::kOneFile) {
		// MCacheFormatDescription::kOneFilePerFrame not yet supported
		// MCacheFormatDescription::kNoFile would that ever happen?
		MGlobal::displayError(
		        "usdCacheFormat::writeDescription: one file per frame mode is not supported");
		return MS::kFailure;
	}
	// Should not happen as with readDescription but still in examples
	// writeDescription opens its own separate description file
	if (not mLayerPtr) {
		MString mayaFileName = descriptionFileLocation + baseFileName;
		this->open(mayaFileName.asChar(), kWrite);
		if (not mLayerPtr) {
			MGlobal::displayError("usdCacheFormat::writeDescription: no open layer");
			return MS::kFailure;
		}
	}
	MTime timePerFrame = description.getTimePerFrame();
	MTime startTime, endTime;
	description.getStartAndEndTimes(startTime, endTime);
	// Maybe should check startTime, endTime and timePerFrame .unit()
	// and use that? (hopefully the 3 being the same...)
	// mTimeUnit = timePerFrame.unit();
	MTime oneSecond(1, MTime::kSeconds);
	double framesPerSecond = oneSecond.asUnits(MTime::uiUnit());
	mLayerPtr->SetFramesPerSecond(framesPerSecond);
	mLayerPtr->SetTimeCodesPerSecond(timePerFrame.asUnits(mTimeUnit) * framesPerSecond);
	// Not sure what that's for
	mLayerPtr->SetFramePrecision(mLayerPtr->GetTimeCodesPerSecond() / mLayerPtr->GetFramesPerSecond());
	mLayerPtr->SetStartTimeCode(startTime.asUnits(mTimeUnit));
	mLayerPtr->SetEndTimeCode(endTime.asUnits(mTimeUnit));

	std::string documentation;
	MStringArray info;
	description.getDescriptionInfo(info);
	for (unsigned i = 0; i < info.length(); ++i) {
		documentation += std::string(info[i].asChar()) + "\n";
	}
	mLayerPtr->SetDocumentation(documentation);

	MStatus status = MS::kSuccess;
	unsigned int channels = description.getNumChannels();
	for (unsigned i = 0; i < channels; ++i) {
		// No clue on how to use that
		// MCacheFormatDescription::CacheSamplingType samplingType = description.getChannelSamplingType(i);
		// MTime samplingRate = description.getChannelSamplingRate(i);
		// MTime startTime = description.getChannelStartTime(i);
		// MTime endTime = description.getChannelEndTime(i);
		SdfPath attrPath = this->addChannel(description.getChannelName(i),
				description.getChannelInterpretation(i),
				description.getChannelDataType(i));
		if (attrPath.IsEmpty()) {
			status = MS::kFailure;
		}
	}
	// Store the channel to path map as layer custom metadata
	this->writeMetadata();

	if (MS::kSuccess == status) {
		mDescript = true;
	}
	return status;
}

MStatus usdCacheFormat::readDescription(MCacheFormatDescription& description,
        const MString& descriptionFileLocation, const MString& baseFileName) {
	// Because Maya can call this directly expecting it to open a separate description file
	if (not mLayerPtr) {
		MString mayaFileName = descriptionFileLocation + baseFileName;
		this->open(mayaFileName.asChar(), kRead);
		if (not mLayerPtr) {
			MGlobal::displayError("usdCacheFormat::readDescription: no open layer");
			return MS::kFailure;
		}
	}
	// MCacheFormatDescription::kOneFilePerFrame not yet supported
	description.setDistribution(MCacheFormatDescription::kOneFile);
	// Store in metadata instead of recalculating, for safety?
	double timePerFrame = mLayerPtr->GetTimeCodesPerSecond()
	        / mLayerPtr->GetFramesPerSecond();
	// Check for problems if cache is written and read in different settings
	// Maya always seems to use this for passed times but should probably
	// be stored as metadata on file during writeDescription ?
	const MTime samplingRate(timePerFrame, mTimeUnit);
	const MTime startTime(mLayerPtr->GetStartTimeCode(), mTimeUnit);
	const MTime endTime(mLayerPtr->GetEndTimeCode(), mTimeUnit);
	description.setTimePerFrame(samplingRate);

	const std::string documentation = mLayerPtr->GetDocumentation();
	const std::vector<std::string> docLines = TfStringSplit(documentation, "\n");
	for (const std::string& docLine : docLines) {
		description.addDescriptionInfo(MString(docLine.c_str()));
	}
	// Parse the channel to path map from metadata
	this->readMetadata();

	MStatus status = MS::kSuccess;
	for (auto& channel : mPathMap) {
		std::string channelName = channel.first;
		SdfPath attrPath = channel.second;
		SdfAttributeSpecHandle attrSpec = mLayerPtr->GetAttributeAtPath(attrPath);
		if (attrSpec) {
			std::string attrName = attrSpec->GetName();
			SdfValueTypeName attrType = attrSpec->GetTypeName();
			MCacheFormatDescription::CacheDataType dataType;
			if (SdfValueTypeNames->Double == attrType) {
				dataType = MCacheFormatDescription::kDouble;
			} else if (SdfValueTypeNames->DoubleArray == attrType) {
				dataType = MCacheFormatDescription::kDoubleArray;
			} else if (SdfValueTypeNames->IntArray == attrType) {
				dataType = MCacheFormatDescription::kInt32Array;
			} else if (SdfValueTypeNames->FloatArray == attrType) {
				dataType = MCacheFormatDescription::kFloatArray;
			} else if (SdfValueTypeNames->Vector3dArray == attrType) {
				dataType = MCacheFormatDescription::kDoubleVectorArray;
			} else if (SdfValueTypeNames->Vector3fArray == attrType) {
				dataType = MCacheFormatDescription::kFloatVectorArray;
			} else {
				MGlobal::displayError("usdCacheFormat::readDescription: Unsupported type for channel "
						+ MString(channelName.c_str()));
				status = MS::kFailure;
				continue;
			}
			MCacheFormatDescription::CacheSamplingType samplingType;
			samplingType = MCacheFormatDescription::kRegular;
			description.addChannel(MString(channelName.c_str()),
			        MString(attrName.c_str()), dataType, samplingType,
			        samplingRate, startTime, endTime, &status);
			if (MS::kSuccess != status) {
				MGlobal::displayError("usdCacheFormat::readDescription: Failed to map channel "
						+ MString(channelName.c_str()));
				status = MS::kFailure;
				continue;
			}
		} else {
			MGlobal::displayError("usdCacheFormat::readDescription: Found no attribute specification for "
					+ MString(channelName.c_str()));
			status = MS::kFailure;
			continue;
		}
	}

	return status;
}

MStatus usdCacheFormat::writeTime(MTime& time) {
	mCurrentTime = time.asUnits(mTimeUnit);
	if ((not mLayerPtr->HasStartTimeCode())
		|| (mCurrentTime < mLayerPtr->GetStartTimeCode())) {
		mLayerPtr->SetStartTimeCode(mCurrentTime);
	}
	if ((not mLayerPtr->HasEndTimeCode())
		|| (mCurrentTime > mLayerPtr->GetEndTimeCode())) {
		mLayerPtr->SetEndTimeCode(mCurrentTime);
	}
	return MS::kSuccess;
}

MStatus usdCacheFormat::readTime(MTime& time) {
	// Never seen it get called in a read cache
	time = MTime(mCurrentTime, mTimeUnit);
	return MS::kSuccess;
}

MStatus usdCacheFormat::findTime(MTime& time, MTime& foundTime) {
	// It the Maya dev kit examples it states
	// "Find the biggest cached time, which is smaller or equal to seekTime and return foundTime"
	double seekTime = time.asUnits(mTimeUnit);
	double lowerTime = std::numeric_limits<double>::infinity();
	double upperTime = std::numeric_limits<double>::infinity();
	mLayerPtr->GetBracketingTimeSamples(seekTime, &lowerTime, &upperTime);
	if (lowerTime <= seekTime) {
		foundTime = MTime(lowerTime, mTimeUnit);
		mCurrentTime = lowerTime;
		return MS::kSuccess;
	} else {
		// this->rewind();
		return MS::kFailure;
	}
}

MStatus usdCacheFormat::readNextTime(MTime& foundTime) {
	// It's 1 at 6000 fps here. Maya seems to only use this setting
	// when communicating with cache, so it's 1/250th of a frame
	// at actual 24fps. Is it small enough for our needs, it must be small
	// enough not to overshoot a possible sub frame sample.
	// Or use UsdTimeCode SafeStep or calculate it from the SdfLayer
	// FramePrecision field ?
	double epsilonTime = 1;
	double seekTime = mCurrentTime;
	double lowerTime = mCurrentTime;
	double upperTime = mCurrentTime;
	if (mLayerPtr->GetBracketingTimeSamples(seekTime + epsilonTime,
	        &lowerTime, &upperTime)) {
		if ((upperTime < std::numeric_limits<double>::infinity())
		        && (upperTime >= mCurrentTime + epsilonTime)) {
			mCurrentTime = upperTime;
			foundTime = MTime(upperTime, mTimeUnit);
			return MS::kSuccess;
		}
	}
	// MGlobal::displayError(MString("usdCacheFormat::readNextTime failure for ")+seekTime);
	return MS::kFailure;
}

MStatus usdCacheFormat::writeChannelName(const MString& name) {
	std::string channel(name.asChar());
	mCurrentChannel = channel;
	auto it = mPathMap.find(channel);
	if (it != mPathMap.end()) {
		mCurrentPath = it->second;
		return MS::kSuccess;
	} else {
		mCurrentPath = SdfPath();
		return MS::kFailure;
	}
	// Need a way for first frame to handle adding the channel
	// when we know what type Maya is trying to write and the exact attribute name
}

MStatus usdCacheFormat::findChannelName(const MString& name) {
	std::string channel = name.asChar();
	auto it = mPathMap.find(channel);
	if (it != mPathMap.end()) {
		mCurrentChannel = it->first;
		mCurrentPath = it->second;
		return MS::kSuccess;
	} else {
		MGlobal::displayError(MString("usdCacheFormat::findChannelName failure for ")+ name);
		return MS::kFailure;
	}
}

MStatus usdCacheFormat::readChannelName(MString& name) {
	// It's actually a find next channel name for Maya, caller is using the MS::kFailure
	// return not as an actual error but as an indication we read last channel
	std::string channel = mCurrentChannel;
	auto it = mPathMap.find(channel);
	if (it != mPathMap.end()) {
		if (++it != mPathMap.end()) {
			mCurrentChannel = it->first;
			mCurrentPath = it->second;
			name = MString(mCurrentChannel.c_str());
			return MS::kSuccess;
		}
	}
	// MGlobal::displayError(MString("usdCacheFormat::readChannelName failure for ")+ name);
	return MS::kFailure;
}

void usdCacheFormat::beginWriteChunk() {
	// Nothing to do
}

void usdCacheFormat::endWriteChunk() {
	// Save after each chunk (frame or sub frame) for safety and memory efficiency
	mLayerPtr->Save();
}

MStatus usdCacheFormat::beginReadChunk() {
	// Nothing to do
	return MS::kSuccess;
}

void usdCacheFormat::endReadChunk() {
	// Gets called after a readChannelName failure (last channel)
	mCurrentChannel = "";
	mCurrentPath = SdfPath();
}

MStatus usdCacheFormat::writeInt32(int i) {
	mLayerPtr->SetTimeSample(mCurrentPath, mCurrentTime, i);
	return MS::kSuccess;
}

int usdCacheFormat::readInt32() {
	int result = 0;
	mLayerPtr->QueryTimeSample(mCurrentPath, mCurrentTime, &result);
	return result;
}

MStatus usdCacheFormat::writeDoubleArray(const MDoubleArray& array) {
	int size = array.length();
	VtArray<double> varray(size);
	array.get(varray.data());
	mLayerPtr->SetTimeSample(mCurrentPath, mCurrentTime, varray);
	return MS::kSuccess;
}

MStatus usdCacheFormat::writeFloatArray(const MFloatArray& array) {
	int size = array.length();
	VtArray<float> varray(size);
	array.get(varray.data());
	mLayerPtr->SetTimeSample(mCurrentPath, mCurrentTime, varray);
	return MS::kSuccess;
}

MStatus usdCacheFormat::writeDoubleVectorArray(const MVectorArray& array) {
	size_t size = array.length();
	VtArray<GfVec3d> varray(size);
	for (size_t i = 0; i < size; i++) {
		varray[i].Set(array[i][0], array[i][1], array[i][2]);
	}
	mLayerPtr->SetTimeSample(mCurrentPath, mCurrentTime, varray);
	return MS::kSuccess;
}

MStatus usdCacheFormat::writeFloatVectorArray(const MFloatVectorArray& array) {
	size_t size = array.length();
	VtArray<GfVec3f> varray(size);
	for (size_t i = 0; i < size; i++) {
		varray[i].Set(array[i][0], array[i][1], array[i][2]);
	}
	mLayerPtr->SetTimeSample(mCurrentPath, mCurrentTime, varray);
	return MS::kSuccess;
}

unsigned usdCacheFormat::readArraySize() {
	// Might be able to pass back a default size and wait until the actual read for resizing
	unsigned result = 0;
	SdfAttributeSpecHandle attrSpec = mLayerPtr->GetAttributeAtPath(mCurrentPath);
	if (attrSpec) {
		std::string attrName = attrSpec->GetName();
		SdfValueTypeName attrType = attrSpec->GetTypeName();
		if (SdfValueTypeNames->Double == attrType) {
			result = 1;
		} else if (SdfValueTypeNames->DoubleArray == attrType) {
			VtArray<double> varray;
			mLayerPtr->QueryTimeSample(mCurrentPath, mCurrentTime, &varray);
			result = varray.size();
		} else if (SdfValueTypeNames->IntArray == attrType) {
			VtArray<int> varray;
			mLayerPtr->QueryTimeSample(mCurrentPath, mCurrentTime, &varray);
			result = varray.size();
		} else if (SdfValueTypeNames->FloatArray == attrType) {
			VtArray<float> varray;
			mLayerPtr->QueryTimeSample(mCurrentPath, mCurrentTime, &varray);
			result = varray.size();
		} else if (SdfValueTypeNames->Vector3dArray == attrType) {
			VtArray<GfVec3d> varray;
			mLayerPtr->QueryTimeSample(mCurrentPath, mCurrentTime, &varray);
			result = varray.size();
		} else if (SdfValueTypeNames->Vector3fArray == attrType) {
			VtArray<GfVec3f> varray;
			mLayerPtr->QueryTimeSample(mCurrentPath, mCurrentTime, &varray);
			result = varray.size();
		} else {
			MGlobal::displayError("usdCacheFormat::readArraySize: Unsupported type for attribute "
					+ MString(attrName.c_str()));
		}
	}
	return result;
}

MStatus usdCacheFormat::readDoubleArray(MDoubleArray& array,
        unsigned arraySize) {
	VtArray<double> varray;
	mLayerPtr->QueryTimeSample(mCurrentPath, mCurrentTime, &varray);
	size_t size = varray.size();
	array.setLength (size);
	for (size_t i = 0; i < size; i++) {
		array[i] = varray[i];
	}
	arraySize = size;
	return MS::kSuccess;
}

MStatus usdCacheFormat::readFloatArray(MFloatArray& array,
		unsigned arraySize) {
	VtArray<float> varray;
	mLayerPtr->QueryTimeSample(mCurrentPath, mCurrentTime, &varray);
	size_t size = varray.size();
	array.setLength (size);
	for (size_t i = 0; i < size; i++) {
		array[i] = varray[i];
	}
	arraySize = size;
	return MS::kSuccess;
}

MStatus usdCacheFormat::readDoubleVectorArray(MVectorArray& array,
        unsigned arraySize) {
	VtArray<GfVec3d> varray;
	mLayerPtr->QueryTimeSample(mCurrentPath, mCurrentTime, &varray);
	size_t size = varray.size();
	array.setLength (size);
	for (size_t i = 0; i < size; i++) {
		array.set(varray[i].GetArray(), i);
	}
	arraySize = size;
	return MS::kSuccess;
}

MStatus usdCacheFormat::readFloatVectorArray(MFloatVectorArray& array,
        unsigned arraySize) {
	VtArray<GfVec3f> varray;
	mLayerPtr->QueryTimeSample(mCurrentPath, mCurrentTime, &varray);
	size_t size = varray.size();
	array.setLength (size);
	for (size_t i = 0; i < size; i++) {
		array.set(varray[i].GetArray(), i);
	}
	arraySize = size;
	return MS::kSuccess;
}

