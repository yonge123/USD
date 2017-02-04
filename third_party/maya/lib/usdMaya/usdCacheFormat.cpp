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
#include "pxr/usd/sdf/childrenView.h"
#include "pxr/usd/usdGeom/points.h"

TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaCacheFormatTokens,
                        PXRUSDMAYA_CACHEFORMAT_TOKENS);


// Some static inits
const std::string usdCacheFormat::mTranslatorName = "pxrUsdCacheFormat";
const std::string usdCacheFormat::mDefaultPrimName = "cache";
const std::string usdCacheFormat::mDefaultPrimType = "Points";
const MTime::Unit usdCacheFormat::mTimeUnit = MTime::k6000FPS;

// Could populate that with GetSchemaAttributeNames but can we get the
// pre declated attributes types the same way?
const usdCacheFormat::attributesSet usdCacheFormat::mDefinedAttributes = {
	usdCacheFormat::attributeDefinition("id",
	                                    MCacheFormatDescription::kDoubleArray,
	                                    "ids",
	                                    SdfValueTypeNames->Int64Array),
	usdCacheFormat::attributeDefinition("radiusPP",
	                                    MCacheFormatDescription::kDoubleArray,
	                                    "widths",
	                                    SdfValueTypeNames->FloatArray,
	                                    [](VtValue x) -> VtValue { return VtValue(2.0 * x.Get<double>()); },
										[](VtValue x) -> VtValue { return VtValue(0.5f * x.Get<float>()); }),
	usdCacheFormat::attributeDefinition("position",
	                                    MCacheFormatDescription::kFloatVectorArray,
	                                    "points",
	                                    SdfValueTypeNames->Point3fArray),
	usdCacheFormat::attributeDefinition("velocity",
	                                    MCacheFormatDescription::kFloatVectorArray,
	                                    "velocities",
	                                    SdfValueTypeNames->Vector3fArray),
	usdCacheFormat::attributeDefinition("userVector3PP",
	                                    MCacheFormatDescription::kFloatVectorArray,
	                                    "normals",
	                                    SdfValueTypeNames->Normal3fArray),
	usdCacheFormat::attributeDefinition("rgbPP",
	                                    MCacheFormatDescription::kFloatVectorArray,
	                                    "primvarsDisplayColor",
	                                    SdfValueTypeNames->Color3fArray),
	usdCacheFormat::attributeDefinition("opacityPP",
	                                    MCacheFormatDescription::kDoubleArray,
	                                    "primvarsDisplayOpacity",
	                                    SdfValueTypeNames->FloatArray)
};

usdCacheFormat::usdCacheFormat() {
}

usdCacheFormat::~usdCacheFormat() {
}

MString usdCacheFormat::translatorName() {
	// For presentation in GUI
	return MString(mTranslatorName.c_str());
}

MString usdCacheFormat::extension() {
	// For filtering display of files on disk
	return MString(
	        PxrUsdMayaCacheFormatTokens->UsdFileExtensionDefault.GetText());
}

bool usdCacheFormat::handlesDescription() {
	return false;
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
			if (mLayerPtr && not this->isValid()) {
				this->closeLayer();
			}
		}
		if (not mLayerPtr) {
			mLayerPtr = SdfLayer::CreateNew(iFileName);
		}
		if (mLayerPtr) {
			mLayerPtr->SetPermissionToEdit(true);
			mLayerPtr->SetPermissionToSave(true);
			// Add cache prim right away because writing cache frame 0 will
			// happen before writeDefinition is called for the first time
			const SdfPath specPath = this->findOrAddDefaultPrim();
			if (not specPath.IsPrimPath()) {
				MGlobal::displayError(
					"usdCacheFormat::open: (write) could not find or create the cache default prim.");
				return MS::kFailure;
			}
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
			if (this->readHeader()) {
				mLayerPtr->SetPermissionToEdit(false);
				mLayerPtr->SetPermissionToSave(false);
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
		mCachedAttributes.clear();
		mLayerPtr.Reset();
	}
}

void usdCacheFormat::close()
{
	// Maya can call close file directly but we need to control when we dump a layer if ever
}

MStatus usdCacheFormat::isValid() {
	// Checking that it has a correct header is done just once at open
	if (mLayerPtr) {
		return MS::kSuccess;
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
		// When writeHeader is called, we get a startTime of 0 and endTime of 1 ?
		mLayerPtr->SetStartTimeCode(startTime.asUnits(mTimeUnit));
		mLayerPtr->SetEndTimeCode(endTime.asUnits(mTimeUnit));
		mCurrentTime = startTime.asUnits(mTimeUnit);
		MTime oneSecond(1, MTime::kSeconds);
		double framesPerSecond = oneSecond.asUnits(MTime::uiUnit());
		double timesCodesPerSecond = oneSecond.asUnits(mTimeUnit);
		double timeCodesPerFrame = timesCodesPerSecond / framesPerSecond;
		mLayerPtr->SetFramesPerSecond(framesPerSecond);
		mLayerPtr->SetTimeCodesPerSecond(timesCodesPerSecond);
		mLayerPtr->SetFramePrecision(timeCodesPerFrame);
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
			// If it looks like a proper cache file, read the attributes defined in it
			return this->readExistingAttributes();
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

SdfPath usdCacheFormat::findOrAddDefaultPrim() {
	// We assume one node per file, fixed name used in cache
	if (mLayerPtr->HasDefaultPrim()) {
		return SdfPath(mLayerPtr->GetDefaultPrim());
	} else {
		SdfPrimSpecHandle primSpec = mLayerPtr->GetPrimAtPath(SdfPath(mDefaultPrimName));
		if (not primSpec) {
			primSpec = SdfPrimSpec::New(mLayerPtr, mDefaultPrimName, SdfSpecifierDef, mDefaultPrimType);
		}
		if (primSpec) {
			mLayerPtr->SetDefaultPrim(primSpec->GetNameToken());
			SdfPath primPath = primSpec->GetPath();
			// Add predefined attributes
			return primPath;
		} else {
			return SdfPath();
		}
	}
}

SdfValueTypeName usdCacheFormat::MCacheDataTypeToSdfValueTypeName(
	const MCacheFormatDescription::CacheDataType dataType) {
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
		case MCacheFormatDescription::kUnknownData:
		default:
			break;

	}
	return attrType;
}

// It's not complete, just handles the cases we need for the custom attrbites
// that can be created by usdCacheFormat
MCacheFormatDescription::CacheDataType usdCacheFormat::SdfValueTypeNameToMCacheDataType(
	const SdfValueTypeName& attrType) {
	MCacheFormatDescription::CacheDataType dataType;
	if (attrType == SdfValueTypeNames->Double) {
		dataType = MCacheFormatDescription::kDouble;
	} else if (attrType == SdfValueTypeNames->DoubleArray) {
		dataType = MCacheFormatDescription::kDoubleArray;
	} else if (attrType == SdfValueTypeNames->IntArray) {
		dataType = MCacheFormatDescription::kInt32Array;
	} else if (attrType == SdfValueTypeNames->FloatArray) {
		dataType = MCacheFormatDescription::kFloatArray;
	} else if (attrType == SdfValueTypeNames->FloatArray) {
		dataType = MCacheFormatDescription::kFloatArray;
	} else if (attrType == SdfValueTypeNames->Vector3dArray) {
		dataType = MCacheFormatDescription::kDoubleVectorArray;
	} else if (attrType == SdfValueTypeNames->Vector3fArray) {
		dataType = MCacheFormatDescription::kFloatVectorArray;
	} else {
		dataType = MCacheFormatDescription::kUnknownData;
	}
	return dataType;
}

// convenience methods with no checking
// We will need something better if we have attributes with _ in their name
std::string usdCacheFormat::mayaNodeFromChannel(const std::string& channel)
{
	return TfStringGetBeforeSuffix(channel, '_');
}

std::string usdCacheFormat::mayaAttrFromChannel(const std::string& channel)
{
	return TfStringGetSuffix(channel, '_');
}

SdfPath usdCacheFormat::usdPathFromAttribute(const TfToken& usdAttrName)
{
	SdfPath primPath(mLayerPtr->GetDefaultPrim());
	primPath = primPath.MakeAbsolutePath(SdfPath("/"));
	SdfPath attrPath = primPath.AppendProperty(usdAttrName);
	return attrPath;
}

usdCacheFormat::attrById::iterator usdCacheFormat::findOrAddMayaAttribute(
	const std::string& mayaAttrName,
	const MCacheFormatDescription::CacheDataType mayaDataType) {

	usdCacheFormat::attrById::iterator foundIt = mCachedAttributes.end();

	auto itName = mCachedAttributes.get<maya>().find(mayaAttrName);
	if (itName != mCachedAttributes.get<maya>().end()) {
		// We found a cached attribute of that name, sanity check on type
		if (mayaDataType == itName->mayaDataType) {
			foundIt = mCachedAttributes.project<0>(itName);
		} else {
			MGlobal::displayError(
				"usdCacheFormat::findOrAddMayaAttribute: Attribute type mismatch on cached attribute "
				+ MString(mayaAttrName.c_str()));
			return mCachedAttributes.end();
		}
	} else {
		// We don't have that attribute in the list, we'll register it (for kWrite)
		usdCacheFormat::attributeDefinition attrDef;
		auto itDefined = mDefinedAttributes.get<maya>().find(mayaAttrName);
		if (itDefined != mDefinedAttributes.get<maya>().end()) {
			// We found a defined attribute of that name, sanity check on type
			if (mayaDataType == itDefined->mayaDataType) {
				attrDef = *itDefined;
			} else {
				MGlobal::displayError(
					"usdCacheFormat::findOrAddMayaAttribute: Attribute type mismatch on defined attribute "
					+ MString(mayaAttrName.c_str()));
				return mCachedAttributes.end();
			}
		} else {
			// Build a description for it
			if (not SdfAttributeSpec::IsValidName(mayaAttrName)) {
				MGlobal::displayError(
					"usdCacheFormat::findOrAddMayaAttribute: Invalid attribute name "
					+ MString(mayaAttrName.c_str()));
				return mCachedAttributes.end();
			}
			SdfValueTypeName usdAttrType = this->MCacheDataTypeToSdfValueTypeName(mayaDataType);
			if (not usdAttrType) {
				MGlobal::displayError(
					"usdCacheFormat::findOrAddMayaAttribute: Unknown attribute type for "
					+ MString(mayaAttrName.c_str()));
				return mCachedAttributes.end();
			}
			attrDef.mayaAttrName = mayaAttrName;
            attrDef.mayaDataType = mayaDataType;
			attrDef.usdAttrName = TfToken(mayaAttrName);
			attrDef.usdAttrType = usdAttrType;
		}
		// Actually add it on the defaultPrim
		SdfPath primPath(mLayerPtr->GetDefaultPrim());
		SdfPrimSpecHandle primSpec = mLayerPtr->GetPrimAtPath(primPath);
		if (not primSpec) {
			MGlobal::displayError(
				"usdCacheFormat::findOrAddMayaAttribute: Invalid default prim path "
				+ MString(primPath.GetString().c_str()));
			return mCachedAttributes.end();
		}
		SdfPath attrPath = this->usdPathFromAttribute(attrDef.usdAttrName);
		SdfAttributeSpecHandle attrSpec = mLayerPtr->GetAttributeAtPath(attrPath);
		if (attrSpec) {
			if (attrSpec->GetTypeName() != attrDef.usdAttrType) {
				MGlobal::displayError(
					"usdCacheFormat::findOrAddAttribute: Attribute type mismatch on existing attribute "
					+ MString(attrPath.GetString().c_str()));
				return mCachedAttributes.end();
			}
		} else {
			// I can get null returns though attribute is there when checking exported file?
			attrSpec = SdfAttributeSpec::New(primSpec,
			                                 attrDef.usdAttrName,
			                                 attrDef.usdAttrType,
			                                 SdfVariabilityVarying,
			                                 false);
			// Must be ordered in the order Maya will request them
			primSpec->InsertInPropertyOrder(attrSpec->GetNameToken());
		}
		// If add worked, add it to the cached attributes set
		if (mLayerPtr->GetAttributeAtPath(attrPath)) {
			mCachedAttributes.push_back(attrDef);
			foundIt = std::prev(mCachedAttributes.end());
		} else {
			MGlobal::displayError(
				"usdCacheFormat::findOrAddMayaAttribute: failed to add attribute "
				+ MString(mayaAttrName.c_str()));
			return mCachedAttributes.end();
		}
	}

	return foundIt;
}

usdCacheFormat::attrById::iterator usdCacheFormat::findOrAddUsdAttribute(const TfToken& usdAttrName,
                                         const SdfValueTypeName& usdAttrType)
{
	usdCacheFormat::attrById::iterator foundIt = mCachedAttributes.end();

	auto itName = mCachedAttributes.get<usd>().find(usdAttrName);
	if (itName != mCachedAttributes.get<usd>().end()) {
		// We found a cached attribute of that name, sanity check on type
		if (usdAttrType == itName->usdAttrType) {
			foundIt = mCachedAttributes.project<0>(itName);
		} else {
			MGlobal::displayError(
				"usdCacheFormat::findOrAddUsdAttribute: Attribute type mismatch on cached attribute "
				+ MString(usdAttrName.GetString().c_str()));
			return mCachedAttributes.end();
		}
	} else {
		// We don't have that attribute in the list, we'll register it (for kRead)
		usdCacheFormat::attributeDefinition attrDef;
		auto itDefined = mDefinedAttributes.get<usd>().find(usdAttrName);
		if (itDefined != mDefinedAttributes.get<usd>().end()) {
			// We found a defined attribute of that name, sanity check on type
			if (usdAttrType == itDefined->usdAttrType) {
				attrDef = *itDefined;
			} else {
				MGlobal::displayError(
					"usdCacheFormat::findOrAddUsdAttribute: Attribute type mismatch on defined attribute "
					+ MString(usdAttrName.GetString().c_str()));
				return mCachedAttributes.end();
			}
		} else {
			// Build a description for it
			MCacheFormatDescription::CacheDataType mayaDataType = this->SdfValueTypeNameToMCacheDataType(usdAttrType);
			if (mayaDataType == MCacheFormatDescription::kUnknownData) {
				MGlobal::displayError(
					"usdCacheFormat::findOrAddUsdAttribute: Unknown or unsupported attribute type for "
					+ MString(usdAttrName.GetString().c_str()));
				return mCachedAttributes.end();
			}
			attrDef.mayaAttrName = usdAttrName;
			attrDef.mayaDataType = mayaDataType;
			attrDef.usdAttrName = usdAttrName;
			attrDef.usdAttrType = usdAttrType;
		}
		// No need to add it on the default prim the goal of this method
		// is to help looking for maya channel / attributes corresponding
		// to existing attributes in a usd cache
		mCachedAttributes.push_back(attrDef);
		foundIt = std::prev(mCachedAttributes.end());
	}

	return foundIt;
}

void usdCacheFormat::writeMetadata()
{
	// Only used for debugging
	VtDictionary attrCachedMeta;
	unsigned int id = 0;
	for (auto it : mCachedAttributes) {
		VtDictionary attrDef;
		attrDef["mayaAttrName"] = it.mayaAttrName.c_str();
		attrDef["mayaDataType"] = TfIntToString(it.mayaDataType);
		attrDef["usdAttrName"] = it.usdAttrName.GetString().c_str();
		attrDef["usdAttrType"] = it.usdAttrType.GetAsToken().GetString().c_str();
		attrDef["convertToUsd"] = (it.convertToUsd != NULL) ? "yes" : "no";
		attrDef["convertFromUsd"] = (it.convertFromUsd != NULL) ? "yes" : "no";
		attrCachedMeta[TfStringPrintf("%02u", id++)] = attrDef ;
	}
	mLayerPtr->SetCustomLayerData(attrCachedMeta);
}

// We need to use the same attributes order as the one that was used
// during write, so either rely on some indexed meta data or defining
// a PropertyOrder when creating attributes on the default prim
MStatus usdCacheFormat::readExistingAttributes()
{
	MStatus status = MS::kSuccess;
	mCachedAttributes.clear();
	SdfPath primPath(mLayerPtr->GetDefaultPrim());
	SdfPrimSpecHandle primSpec = mLayerPtr->GetPrimAtPath(primPath);
	for (const auto attr: primSpec->GetProperties()) {
		SdfPath usdAttrPath = attr->GetPath();
		TfToken usdAttrName = attr->GetNameToken();
		SdfValueTypeName usdAttrType = attr->GetTypeName();
		// Add it to the set of cached attributes
		auto it = this->findOrAddUsdAttribute(usdAttrName, usdAttrType);
		if (it != mCachedAttributes.end()) {
		} else {
			MGlobal::displayError(MString("usdCacheFormat::readExistingAttributes could not add ")
			                     + MString(usdAttrName.GetString().c_str()));
			status = MS::kFailure;
		}
	}
	if (not mCachedAttributes.empty()) {
		return status;
	} else {
		MGlobal::displayError(MString("usdCacheFormat::readExistingAttributes found not attributes on default prim ")
		                     + MString(primPath.GetString().c_str()));
		return MS::kFailure;
	}
}

MStatus usdCacheFormat::writeTime(MTime& time)
{
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
	// Actually adding the attribute will happen later when we know
	// which type Maya is trying to write
	mCurrentChannel = std::string(name.asChar());
	return MS::kSuccess;
}

MStatus usdCacheFormat::findChannelName(const MString& name) {
	// All channels for kRead should already have been added because
	// Maya will call readChannelName immediatly after calling
	// findChannelName on first channel in set
	const std::string channel = name.asChar();
	const std::string mayaAttrName = this->mayaAttrFromChannel(channel);
	auto itName = mCachedAttributes.get<maya>().find(mayaAttrName);
	if (itName != mCachedAttributes.get<maya>().end()) {
		mCurrentChannel = channel;
		return MS::kSuccess;
	} else {
		mCurrentChannel = "";
		MGlobal::displayError(MString("usdCacheFormat::findChannelName failure for ")
		                      + name);
		return MS::kFailure;
	}
}

MStatus usdCacheFormat::readChannelName(MString& name) {
	// It's actually a find next channel name for Maya, caller is using the MS::kFailure
	// return not as an actual error but as an indication we read last channel
	std::string channel = mCurrentChannel;
	std::string mayaNodeName = this->mayaNodeFromChannel(channel);
	std::string mayaAttrName = this->mayaAttrFromChannel(channel);
	auto itName = mCachedAttributes.get<maya>().find(mayaAttrName);
	if (itName != mCachedAttributes.get<maya>().end()) {
		usdCacheFormat::attrById::iterator itId = mCachedAttributes.project<0>(itName);
		if (++itId !=  mCachedAttributes.end()) {
			const std::string nextMayaAttrName = itId->mayaAttrName;
			const std::string nextChannel = TfStringPrintf("%s_%s", mayaNodeName.c_str(), nextMayaAttrName.c_str());
			// Advance by one channel
			mCurrentChannel = nextChannel;
			name = MString(nextChannel.c_str());
			return MS::kSuccess;
		}
	}
	// No more channels
	mCurrentChannel = "";
	name = MString("");
	return MS::kFailure;
}

void usdCacheFormat::beginWriteChunk() {
	// Nothing to do
}

void usdCacheFormat::endWriteChunk() {
	// Save each frame
	mLayerPtr->Save();
}

MStatus usdCacheFormat::beginReadChunk() {
	// Nothing to do
	return MS::kSuccess;
}

void usdCacheFormat::endReadChunk() {
	// Nothing to do
}

template <class T1, typename T2>
VtArray<T2> usdCacheFormat::convertOutArray(
	const T1& array,
	std::function<VtValue(VtValue x)> convertFn = NULL)
{
	size_t size = array.length();
	VtArray<T2> result(size);
	if (convertFn == NULL) {
		for (size_t i=0; i<size; i++) {
			result[i] = VtValue(array[i]).CastToTypeid(typeid(T2)).Get<T2>();
		}
	} else {
		for (size_t i=0; i<size; i++) {
			result[i] = convertFn(VtValue(array[i])).CastToTypeid(typeid(T2)).Get<T2>();
		}
	}
	return result;
}

template <class T1, typename T2>
VtArray<T2> usdCacheFormat::convertOutVectorArray(
	const T1& array,
	std::function<VtValue(VtValue x)> convertFn = NULL)
{
	size_t size = array.length();
	VtArray<T2> result(size);
	if (convertFn == NULL) {
		for (size_t i=0; i<size; i++) {
			result[i].Set(array[i][0], array[i][1], array[i][2]);
		}
	} else {
		T2 vector;
		for (size_t i=0; i<size; i++) {
			vector.Set(array[i][0], array[i][1], array[i][2]);
			result[i] = convertFn(VtValue(vector)).CastToTypeid(typeid(T2)).Get<T2>();
		}
	}
	return result;
}

MStatus usdCacheFormat::writeInt32(int i)
{
	// Try to add on the fly for writing first frame
	// Note: there is no MCacheFormatDescription for int32 and it seems never to
	// be called by Maya anyway
	auto it = this->findOrAddMayaAttribute(this->mayaAttrFromChannel(mCurrentChannel),
	                                       MCacheFormatDescription::kDouble);
	if (it != mCachedAttributes.end()) {
		if (it->convertToUsd == NULL) {
			mLayerPtr->SetTimeSample(this->usdPathFromAttribute(it->usdAttrName),
			                         mCurrentTime,
			                         i);
		} else {
			mLayerPtr->SetTimeSample(this->usdPathFromAttribute(it->usdAttrName),
			                         mCurrentTime,
			                         it->convertToUsd(VtValue(i)));
		}
	    return MS::kSuccess;
	} else {
		return MS::kFailure;
	}
}

int usdCacheFormat::readInt32() {
	int result = 0;
	auto it = this->findOrAddMayaAttribute(this->mayaAttrFromChannel(mCurrentChannel),
	                                       MCacheFormatDescription::kDouble);
	if (it != mCachedAttributes.end()) {
		VtValue value(result);
		mLayerPtr->QueryTimeSample(this->usdPathFromAttribute(it->usdAttrName),
		                           mCurrentTime,
		                           &value);
		if (it->convertFromUsd == NULL) {
			result = value.UncheckedGet<int>();
		} else {
			result = it->convertFromUsd(value).UncheckedGet<int>();
		}
	}
	return result;
}

MStatus usdCacheFormat::writeDoubleArray(const MDoubleArray& array) {
	auto it = this->findOrAddMayaAttribute(this->mayaAttrFromChannel(mCurrentChannel),
	                                       MCacheFormatDescription::kDoubleArray);
	if (it != mCachedAttributes.end()) {
		const SdfPath attrPath = this->usdPathFromAttribute(it->usdAttrName);
		const SdfValueTypeName usdAttrType = it->usdAttrType;
		if ((SdfValueTypeNames->DoubleArray == usdAttrType)
			&& (it->convertToUsd == NULL)) {
			// Fast case, 1:1 correspondance in type and value,
			// so we can use a direct memory copy
			VtDoubleArray varray(array.length());
			array.get(varray.data());
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         varray);
		} else if (SdfValueTypeNames->DoubleArray == usdAttrType) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutArray<MDoubleArray, double>(array, it->convertToUsd));
		} else if (SdfValueTypeNames->FloatArray == usdAttrType) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutArray<MDoubleArray, float>(array, it->convertToUsd));
		} else if (SdfValueTypeNames->Int64Array == usdAttrType) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutArray<MDoubleArray, long>(array, it->convertToUsd));
		} else if (SdfValueTypeNames->UInt64Array == usdAttrType) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutArray<MDoubleArray, unsigned long>(array, it->convertToUsd));
		} else {
			return MS::kFailure;
		}
		return MS::kSuccess;
	} else {
		return MS::kFailure;
	}
}

MStatus usdCacheFormat::writeFloatArray(const MFloatArray& array) {
	auto it = this->findOrAddMayaAttribute(this->mayaAttrFromChannel(mCurrentChannel),
	                                       MCacheFormatDescription::kFloatArray);
	if (it != mCachedAttributes.end()) {
		const SdfPath attrPath = this->usdPathFromAttribute(it->usdAttrName);
		const SdfValueTypeName usdAttrType = it->usdAttrType;
		if ((SdfValueTypeNames->FloatArray == usdAttrType)
		    && (it->convertToUsd == NULL)) {
			VtFloatArray varray(array.length());
			array.get(varray.data());
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         varray);
		} else if (SdfValueTypeNames->DoubleArray == usdAttrType) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutArray<MFloatArray, double>(array, it->convertToUsd));
		} else if (SdfValueTypeNames->FloatArray == usdAttrType) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutArray<MFloatArray, float>(array, it->convertToUsd));
		} else if (SdfValueTypeNames->Int64Array == usdAttrType) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutArray<MFloatArray, long>(array, it->convertToUsd));
		} else if (SdfValueTypeNames->UInt64Array == usdAttrType) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutArray<MFloatArray, unsigned long>(array, it->convertToUsd));
		} else {
			return MS::kFailure;
		}
		return MS::kSuccess;
	} else {
		return MS::kFailure;
	}
}

MStatus usdCacheFormat::writeDoubleVectorArray(const MVectorArray& array) {
	auto it = this->findOrAddMayaAttribute(this->mayaAttrFromChannel(mCurrentChannel),
	                                       MCacheFormatDescription::kDoubleVectorArray);
	if (it != mCachedAttributes.end()) {
		const SdfPath attrPath = this->usdPathFromAttribute(it->usdAttrName);
		const SdfValueTypeName usdAttrType = it->usdAttrType;
		if ((SdfValueTypeNames->Vector3dArray == usdAttrType)
		    || (SdfValueTypeNames->Point3dArray == usdAttrType)
		    || (SdfValueTypeNames->Normal3dArray == usdAttrType)
		    || (SdfValueTypeNames->Color3dArray == usdAttrType)) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutVectorArray<MVectorArray, GfVec3d>(array, it->convertToUsd));
		} else if ((SdfValueTypeNames->Vector3fArray == usdAttrType)
		           || (SdfValueTypeNames->Point3fArray == usdAttrType)
		           || (SdfValueTypeNames->Normal3fArray == usdAttrType)
		           || (SdfValueTypeNames->Color3fArray == usdAttrType)) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutVectorArray<MVectorArray, GfVec3f>(array, it->convertToUsd));
		} else {
			return MS::kFailure;
		}
		return MS::kSuccess;
	} else {
		return MS::kFailure;
	}
}

MStatus usdCacheFormat::writeFloatVectorArray(const MFloatVectorArray& array) {
	auto it = this->findOrAddMayaAttribute(this->mayaAttrFromChannel(mCurrentChannel),
	                                       MCacheFormatDescription::kFloatVectorArray);
	if (it != mCachedAttributes.end()) {
		const SdfPath attrPath = this->usdPathFromAttribute(it->usdAttrName);
		const SdfValueTypeName usdAttrType = it->usdAttrType;
		if ((SdfValueTypeNames->Vector3dArray == usdAttrType)
		    || (SdfValueTypeNames->Point3dArray == usdAttrType)
		    || (SdfValueTypeNames->Normal3dArray == usdAttrType)
		    || (SdfValueTypeNames->Color3dArray == usdAttrType)) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutVectorArray<MFloatVectorArray, GfVec3d>(array, it->convertToUsd));
		} else if ((SdfValueTypeNames->Vector3fArray == usdAttrType)
		           || (SdfValueTypeNames->Point3fArray == usdAttrType)
		           || (SdfValueTypeNames->Normal3fArray == usdAttrType)
		           || (SdfValueTypeNames->Color3fArray == usdAttrType)) {
			mLayerPtr->SetTimeSample(attrPath,
			                         mCurrentTime,
			                         this->convertOutVectorArray<MFloatVectorArray, GfVec3f>(array, it->convertToUsd));
		} else {
			return MS::kFailure;
		}
		return MS::kSuccess;
	} else {
		return MS::kFailure;
	}
}

unsigned usdCacheFormat::readArraySize() {
	// Might be able to pass back a default size and wait until the actual read for resizing
	// All attributes should exist already on the defaultPrim since we're doing a read
	unsigned result = 0;
	const std::string mayaAttrName = this->mayaAttrFromChannel(mCurrentChannel);
	auto it = mCachedAttributes.get<maya>().find(mayaAttrName);
	if (it != mCachedAttributes.get<maya>().end()) {
		TfToken usdAttrName = it->usdAttrName;
		SdfValueTypeName usdAttrType = it->usdAttrType;
		SdfPath attrPath = this->usdPathFromAttribute(usdAttrName);
		if (usdAttrType) {
			if (usdAttrType.IsArray()) {
				VtValue varray;
				if (mLayerPtr->QueryTimeSample(attrPath, mCurrentTime, &varray)) {
					result = varray.GetArraySize();
				} else {
					MGlobal::displayError("usdCacheFormat::readArraySize: failed to query time sample for "
					                      + MString(usdAttrName.GetString().c_str()));
				}
			} else {
				result = 1;
			}
		} else {
			MGlobal::displayError("usdCacheFormat::readArraySize: unsupported type for attribute "
					+ MString(usdAttrName.GetString().c_str()));
		}
	} else {
		MGlobal::displayError("usdCacheFormat::readArraySize: cannot find attribute for channel "
		                      + MString(mCurrentChannel.c_str()));
	}
	return result;
}


template <typename T1, class T2>
void usdCacheFormat::convertInArray(const VtValue& value, T2& array, std::function<VtValue(VtValue x)> convertFn)
{
	size_t size = value.GetArraySize();
	array.clear();
	array.setLength(size);
	VtArray<T1> varray = value.Get<VtArray<T1>>();
	if (convertFn == NULL) {
		for (size_t i = 0; i < size; i++) {
			array[i] = varray[i];
		}
	}
	else {
		for (size_t i = 0; i < size; i++) {
			array[i] = convertFn(VtValue(varray[i])).Get<T1>();
		}
	}
}

template <typename T1, class T2>
void usdCacheFormat::convertInVectorArray(const VtValue& value, T2& array, std::function<VtValue(VtValue x)> convertFn)
{
	size_t size = value.GetArraySize();
	array.clear();
	array.setLength(size);
	VtArray<T1> varray = value.Get<VtArray<T1>>();
	if (convertFn == NULL) {
		for (size_t i = 0; i < size; i++) {
			array.set(varray[i].GetArray(), i);
		}
	} else {
		for (size_t i = 0; i < size; i++) {
			array.set(convertFn(VtValue(varray[i])).Get<T1>().GetArray(), i);
		}
	}
}

MStatus usdCacheFormat::readDoubleArray(MDoubleArray& array,
        unsigned arraySize)
{
	const std::string mayaAttrName = this->mayaAttrFromChannel(mCurrentChannel);
	auto it = mCachedAttributes.get<maya>().find(mayaAttrName);
	if (it != mCachedAttributes.get<maya>().end()) {
		SdfPath attrPath = this->usdPathFromAttribute(it->usdAttrName);
		VtValue value;
		if (mLayerPtr->QueryTimeSample(attrPath,
		                               mCurrentTime,
		                               &value)) {
			SdfValueTypeName usdAttrType = it->usdAttrType;
			if (SdfValueTypeNames->DoubleArray == usdAttrType) {
				this->convertInArray<double, MDoubleArray>(value, array, it->convertFromUsd);
			} else if (SdfValueTypeNames->FloatArray == usdAttrType) {
				this->convertInArray<float, MDoubleArray>(value, array, it->convertFromUsd);
			} else if (SdfValueTypeNames->Int64Array == usdAttrType) {
				this->convertInArray<long, MDoubleArray>(value, array, it->convertFromUsd);
			} else if (SdfValueTypeNames->UInt64Array == usdAttrType) {
				this->convertInArray<unsigned long, MDoubleArray>(value, array, it->convertFromUsd);
			} else {
				return MS::kFailure;
			}
			if (array.length() == arraySize) {
				return MS::kSuccess;
			}
		}
	}
	return MS::kFailure;
}

MStatus usdCacheFormat::readFloatArray(MFloatArray& array,
		unsigned arraySize) {
	const std::string mayaAttrName = this->mayaAttrFromChannel(mCurrentChannel);
	auto it = mCachedAttributes.get<maya>().find(mayaAttrName);
	if (it != mCachedAttributes.get<maya>().end()) {
		SdfPath attrPath = this->usdPathFromAttribute(it->usdAttrName);
		VtValue value;
		if (mLayerPtr->QueryTimeSample(attrPath,
		                               mCurrentTime,
		                               &value)) {
			SdfValueTypeName usdAttrType = it->usdAttrType;
			if (SdfValueTypeNames->DoubleArray == usdAttrType) {
				this->convertInArray<double, MFloatArray>(value, array, it->convertFromUsd);
			} else if (SdfValueTypeNames->FloatArray == usdAttrType) {
				this->convertInArray<float, MFloatArray>(value, array, it->convertFromUsd);
			} else if (SdfValueTypeNames->Int64Array == usdAttrType) {
				this->convertInArray<long, MFloatArray>(value, array, it->convertFromUsd);
			} else if (SdfValueTypeNames->UInt64Array == usdAttrType) {
				this->convertInArray<unsigned long, MFloatArray>(value, array, it->convertFromUsd);
			} else {
				return MS::kFailure;
			}
			if (array.length() == arraySize) {
				return MS::kSuccess;
			}
		}
	}
	return MS::kFailure;
}

MStatus usdCacheFormat::readDoubleVectorArray(MVectorArray& array,
        unsigned arraySize) {
	const std::string mayaAttrName = this->mayaAttrFromChannel(mCurrentChannel);
	auto it = mCachedAttributes.get<maya>().find(mayaAttrName);
	if (it != mCachedAttributes.get<maya>().end()) {
		SdfPath attrPath = this->usdPathFromAttribute(it->usdAttrName);
		VtValue value;
		if (mLayerPtr->QueryTimeSample(attrPath,
		                               mCurrentTime,
		                               &value)) {
			SdfValueTypeName usdAttrType = it->usdAttrType;
			if ((SdfValueTypeNames->Vector3dArray == usdAttrType)
			    || (SdfValueTypeNames->Point3dArray == usdAttrType)
			    || (SdfValueTypeNames->Normal3dArray == usdAttrType)
			    || (SdfValueTypeNames->Color3dArray == usdAttrType)) {
				this->convertInVectorArray<GfVec3d, MVectorArray>(value, array, it->convertFromUsd);
			} else if ((SdfValueTypeNames->Vector3fArray == usdAttrType)
			           || (SdfValueTypeNames->Point3fArray == usdAttrType)
			           || (SdfValueTypeNames->Normal3fArray == usdAttrType)
			           || (SdfValueTypeNames->Color3fArray == usdAttrType)) {
				this->convertInVectorArray<GfVec3f, MVectorArray>(value, array, it->convertFromUsd);
			} else {
				return MS::kFailure;
			}
			if (array.length() == arraySize) {
				return MS::kSuccess;
			}
		}
	}
	return MS::kFailure;
}

MStatus usdCacheFormat::readFloatVectorArray(MFloatVectorArray& array,
        unsigned arraySize) {
	const std::string mayaAttrName = this->mayaAttrFromChannel(mCurrentChannel);
	auto it = mCachedAttributes.get<maya>().find(mayaAttrName);
	if (it != mCachedAttributes.get<maya>().end()) {
		SdfPath attrPath = this->usdPathFromAttribute(it->usdAttrName);
		VtValue value;
		if (mLayerPtr->QueryTimeSample(attrPath,
		                               mCurrentTime,
		                               &value)) {
			SdfValueTypeName usdAttrType = it->usdAttrType;
			if ((SdfValueTypeNames->Vector3dArray == usdAttrType)
			    || (SdfValueTypeNames->Point3dArray == usdAttrType)
			    || (SdfValueTypeNames->Normal3dArray == usdAttrType)
			    || (SdfValueTypeNames->Color3dArray == usdAttrType)) {
				this->convertInVectorArray<GfVec3d, MFloatVectorArray>(value, array, it->convertFromUsd);
			} else if ((SdfValueTypeNames->Vector3fArray == usdAttrType)
			           || (SdfValueTypeNames->Point3fArray == usdAttrType)
			           || (SdfValueTypeNames->Normal3fArray == usdAttrType)
			           || (SdfValueTypeNames->Color3fArray == usdAttrType)) {
				this->convertInVectorArray<GfVec3f, MFloatVectorArray>(value, array, it->convertFromUsd);
			} else {
				return MS::kFailure;
			}
			if (array.length() == arraySize) {
				return MS::kSuccess;
			}
		}
	}
	return MS::kFailure;
}

