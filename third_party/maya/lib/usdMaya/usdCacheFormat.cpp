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
#include "usdMaya/usdCacheFormat.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/childrenView.h"
#include "pxr/usd/usdGeom/points.h"

#include <maya/MGlobal.h>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    // Some constants
    constexpr auto _defaultPrimName = "cache";
    constexpr auto _defaultPrimType = "Points";
    constexpr auto _timeUnit = MTime::k6000FPS;

    // some utility functions
    // We will need something better if we have attributes with _ in their name
    inline std::string _mayaNodeFromChannel(const std::string& channel)
    {
        return TfStringGetBeforeSuffix(channel, '_');
    }

    inline std::string _mayaAttrFromChannel(const std::string& channel)
    {
        return TfStringGetSuffix(channel, '_');
    }

    // Templates for all the different cases of type conversion
    template <typename T1, typename T2>
    VtArray<T2> _convertOutArray(
        const T1& array,
        const std::function<VtValue(VtValue x)> convertFn = nullptr)
    {
        const auto size = array.length();
        VtArray<T2> result(size);
        if (convertFn == nullptr) {
            for (size_t i = 0; i < size; i++) {
                result[i] = VtValue(array[i]).CastToTypeid(typeid(T2)).Get<T2>();
            }
        } else {
            for (size_t i = 0; i < size; i++) {
                result[i] = convertFn(VtValue(array[i])).CastToTypeid(typeid(T2)).Get<T2>();
            }
        }
        return result;
    }
    template <typename T1, typename T2>
    VtArray<T2> _convertOutVectorArray(
        const T1& array,
        const std::function<VtValue(VtValue x)> convertFn = nullptr)
    {
        const auto size = array.length();
        VtArray<T2> result(size);
        if (convertFn == nullptr) {
            for (size_t i = 0; i < size; i++) {
                result[i].Set(array[i][0], array[i][1], array[i][2]);
            }
        } else {
            T2 vector;
            for (size_t i = 0; i < size; i++) {
                vector.Set(array[i][0], array[i][1], array[i][2]);
                result[i] = convertFn(VtValue(vector)).CastToTypeid(typeid(T2)).Get<T2>();
            }
        }
        return result;
    }

    template <typename T1, typename T2>
    void _convertInArray(const VtValue& value,
                         T2& array,
                         const std::function<VtValue(VtValue x)> convertFn)
    {
        const auto size = value.GetArraySize();
        array.clear();
        array.setLength(size);
        VtArray<T1> varray = value.Get<VtArray<T1>>();
        if (convertFn == nullptr) {
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

    template <typename T1, typename T2>
    void _convertInVectorArray(const VtValue& value,
                               T2& array,
                               const std::function<VtValue(VtValue x)> convertFn)
    {
        const auto size = value.GetArraySize();
        array.clear();
        array.setLength(size);
        VtArray<T1> varray = value.Get<VtArray<T1>>();
        if (convertFn == nullptr) {
            for (size_t i = 0; i < size; i++) {
                array.set(varray[i].GetArray(), i);
            }
        } else {
            for (size_t i = 0; i < size; i++) {
                array.set(convertFn(VtValue(varray[i])).Get<T1>().GetArray(), i);
            }
        }
    }

    // It's not complete, just handles the cases we need for the custom attrbites
    // that can be created on request by UsdCacheFormat. Pre-defined attributes
    // come with a hardcoded type correspondance anyway
    inline
    SdfValueTypeName _mCacheDataTypeToSdfValueTypeName(
        const MCacheFormatDescription::CacheDataType dt) {
        if (dt == MCacheFormatDescription::kDouble) {
            return SdfValueTypeNames->Double;
        } else if (dt == MCacheFormatDescription::kDoubleArray) {
            return SdfValueTypeNames->DoubleArray;
        } else if (dt == MCacheFormatDescription::kInt32Array) {
            return SdfValueTypeNames->IntArray;
        } else if (dt == MCacheFormatDescription::kFloatArray) {
            return SdfValueTypeNames->FloatArray;
        } else if (dt == MCacheFormatDescription::kDoubleVectorArray) {
            return SdfValueTypeNames->Vector3dArray;
        } else if (dt == MCacheFormatDescription::kFloatVectorArray) {
            return SdfValueTypeNames->Vector3fArray;
        } else {
            return SdfValueTypeName();
        }
    }

    inline
    MCacheFormatDescription::CacheDataType _sdfValueTypeNameToMCacheDataType(
        const SdfValueTypeName& at) {
        if (at == SdfValueTypeNames->Double) {
            return MCacheFormatDescription::kDouble;
        } else if (at == SdfValueTypeNames->DoubleArray) {
            return MCacheFormatDescription::kDoubleArray;
        } else if (at == SdfValueTypeNames->IntArray) {
            return MCacheFormatDescription::kInt32Array;
        } else if (at == SdfValueTypeNames->FloatArray) {
            return MCacheFormatDescription::kFloatArray;
        } else if (at == SdfValueTypeNames->FloatArray) {
            return MCacheFormatDescription::kFloatArray;
        } else if (at == SdfValueTypeNames->Vector3dArray) {
            return MCacheFormatDescription::kDoubleVectorArray;
        } else if (at == SdfValueTypeNames->Vector3fArray) {
            return MCacheFormatDescription::kFloatVectorArray;
        } else {
            return MCacheFormatDescription::kUnknownData;
        }
    }

    inline
    SdfPath _usdPathFromAttribute(SdfLayerRefPtr& layerPtr, const TfToken& usdAttrName)
    {
        SdfPath primPath(layerPtr->GetDefaultPrim());
        return primPath.MakeAbsolutePath(SdfPath("/")).AppendProperty(usdAttrName);
    }

    inline
    SdfPath _findOrAddDefaultPrim(SdfLayerRefPtr& layerPtr) {
        // We assume one node per file, fixed name used in cache
        if (layerPtr->HasDefaultPrim()) {
            return SdfPath(layerPtr->GetDefaultPrim());
        } else {
            SdfPrimSpecHandle primSpec = layerPtr->GetPrimAtPath(SdfPath(_defaultPrimName));
            if (!primSpec) {
                primSpec = SdfPrimSpec::New(layerPtr, _defaultPrimName, SdfSpecifierDef, _defaultPrimType);
            }
            if (primSpec) {
                layerPtr->SetDefaultPrim(primSpec->GetNameToken());
                return primSpec->GetPath();
            } else {
                return SdfPath();
            }
        }
    }

    // Attributes correspondance and type do not follow a systematic rule
    // so we need some structure to store their specificities.
    // We will have to access sequentially,since order of creation is important
    // for Maya readChannelName, but also by Maya name or USD name for some calls
    struct _AttributeDefinition {
        std::string mayaAttrName = "";
        MCacheFormatDescription::CacheDataType mayaDataType = MCacheFormatDescription::kUnknownData;
        TfToken usdAttrName = TfToken();
        SdfValueTypeName usdAttrType = SdfValueTypeName();
        std::function<VtValue(VtValue x)> convertToUsd = nullptr;
        std::function<VtValue(VtValue x)> convertFromUsd = nullptr;

        _AttributeDefinition(const std::string& mayaAttrName = "",
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
    struct _Maya{};
    struct _Usd{};
    using attributesSet = boost::multi_index::multi_index_container<
        _AttributeDefinition,
        boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<_Maya>,
                boost::multi_index::member<
                    _AttributeDefinition,
                    std::string,
                    &_AttributeDefinition::mayaAttrName
                >
            >,
            boost::multi_index::ordered_unique<
                boost::multi_index::tag<_Usd>,
                boost::multi_index::member<
                    _AttributeDefinition,
                    TfToken,
                    &_AttributeDefinition::usdAttrName
                >
            >
        >
    >;

    using attrById = attributesSet::nth_index<0>::type;

    // Declare a set of predefined attributes for the specific cases
    // ie usd 'ids' are maya's 'id', usd 'points' are maya's 'position' etc..
    const attributesSet _definedAttributes = {
        {
            "id",
            MCacheFormatDescription::kDoubleArray,
            "ids",
            SdfValueTypeNames->Int64Array
        },
        {
            "radiusPP",
            MCacheFormatDescription::kDoubleArray,
            "widths",
            SdfValueTypeNames->FloatArray,
            [](VtValue x) -> VtValue { return VtValue(2.0 * x.Get<double>()); },
            [](VtValue x) -> VtValue { return VtValue(0.5f * x.Get<float>()); }
        },
        {
            "position",
            MCacheFormatDescription::kFloatVectorArray,
            "points",
            SdfValueTypeNames->Point3fArray
        },
        {
            "velocity",
            MCacheFormatDescription::kFloatVectorArray,
            "velocities",
            SdfValueTypeNames->Vector3fArray
        },
        {
            "userVector3PP",
            MCacheFormatDescription::kFloatVectorArray,
            "normals",
            SdfValueTypeNames->Normal3fArray
        },
        {
            "rgbPP",
            MCacheFormatDescription::kFloatVectorArray,
            "primvarsDisplayColor",
            SdfValueTypeNames->Color3fArray
        },
        {
            "opacityPP",
            MCacheFormatDescription::kDoubleArray,
            "primvarsDisplayOpacity",
            SdfValueTypeNames->FloatArray
        }
    };
}

struct UsdCacheFormat::Impl {
    // Dynamic set for keeping track of created attributes
    attributesSet cachedAttributes;
    attrById::iterator firstMayaAttr;

    inline
    attrById::iterator findOrAddMayaAttribute(
        SdfLayerRefPtr& layerPtr,
        const std::string& mayaAttrName,
        const MCacheFormatDescription::CacheDataType mayaDataType) {

        auto foundIt = cachedAttributes.end();

        const auto itName = cachedAttributes.get<_Maya>().find(mayaAttrName);
        if (itName != cachedAttributes.get<_Maya>().end()) {
            // We found a cached attribute of that name, sanity check on type
            if (mayaDataType == itName->mayaDataType) {
                foundIt = cachedAttributes.project<0>(itName);
            } else {
                TF_WARN("Attribute type mismatch on cached attribute %s", mayaAttrName.c_str());
                return cachedAttributes.end();
            }
        } else {
            // We don't have that attribute in the list, we'll register it (for kWrite)
            _AttributeDefinition attrDef;
            const auto itDefined = _definedAttributes.get<_Maya>().find(mayaAttrName);
            if (itDefined != _definedAttributes.get<_Maya>().end()) {
                // We found a defined attribute of that name, sanity check on type
                if (mayaDataType == itDefined->mayaDataType) {
                    attrDef = *itDefined;
                } else {
                    TF_WARN("Attribute type mismatch on defined attribute %s", mayaAttrName.c_str());
                    return cachedAttributes.end();
                }
            } else {
                // Build a description for it
                if (!SdfAttributeSpec::IsValidName(mayaAttrName)) {
                    TF_WARN("Invalid attribute name %s", mayaAttrName.c_str());
                    return cachedAttributes.end();
                }
                const auto usdAttrType = _mCacheDataTypeToSdfValueTypeName(mayaDataType);
                if (!usdAttrType) {
                    TF_WARN("Unknown attribute type for %s", mayaAttrName.c_str());
                    return cachedAttributes.end();
                }
                attrDef.mayaAttrName = mayaAttrName;
                attrDef.mayaDataType = mayaDataType;
                attrDef.usdAttrName = TfToken(mayaAttrName);
                attrDef.usdAttrType = usdAttrType;
            }
            // Actually add it on the defaultPrim
            SdfPath primPath(layerPtr->GetDefaultPrim());
            SdfPrimSpecHandle primSpec = layerPtr->GetPrimAtPath(primPath);
            if (!primSpec) {
                TF_WARN("Invalid default prim path %s", primPath.GetText());
                return cachedAttributes.end();
            }
            const auto attrPath = _usdPathFromAttribute(layerPtr, attrDef.usdAttrName);
            auto attrSpec = layerPtr->GetAttributeAtPath(attrPath);
            if (attrSpec) {
                if (attrSpec->GetTypeName() != attrDef.usdAttrType) {
                    TF_WARN("Attribute type mismatch on existing attribute %s", attrPath.GetText());
                    return cachedAttributes.end();
                }
            } else {
                // I can get nullptr returns though attribute is there when checking exported file?
                attrSpec = SdfAttributeSpec::New(primSpec,
                                                 attrDef.usdAttrName,
                                                 attrDef.usdAttrType,
                                                 SdfVariabilityVarying,
                                                 false);
                // re enable if order is a problem
                // primSpec->InsertInPropertyOrder(attrSpec->GetNameToken());
            }
            // If add worked, add it to the cached attributes set
            if (layerPtr->GetAttributeAtPath(attrPath)) {
                cachedAttributes.push_back(attrDef);
                foundIt = std::prev(cachedAttributes.end());
            } else {
                TF_WARN("Failed to add attribute %s", mayaAttrName.c_str());
                return cachedAttributes.end();
            }
        }

        return foundIt;
    }

    inline
    attrById::iterator findOrAddUsdAttribute(
        const TfToken& usdAttrName,
        const SdfValueTypeName& usdAttrType)
    {
        attrById::iterator foundIt = cachedAttributes.end();

        const auto itName = cachedAttributes.get<_Usd>().find(usdAttrName);
        if (itName != cachedAttributes.get<_Usd>().end()) {
            // We found a cached attribute of that name, sanity check on type
            if (usdAttrType == itName->usdAttrType) {
                foundIt = cachedAttributes.project<0>(itName);
            } else {
                TF_WARN("Attribute type mismatch on cached attribute %s", usdAttrName.GetText());
                return cachedAttributes.end();
            }
        } else {
            // We don't have that attribute in the list, we'll register it (for kRead)
            _AttributeDefinition attrDef;
            const auto itDefined = _definedAttributes.get<_Usd>().find(usdAttrName);
            if (itDefined != _definedAttributes.get<_Usd>().end()) {
                // We found a defined attribute of that name, sanity check on type
                if (usdAttrType == itDefined->usdAttrType) {
                    attrDef = *itDefined;
                } else {
                    TF_WARN("Attribute type mismatch on defined attribute %s", usdAttrName.GetText());
                    return cachedAttributes.end();
                }
            } else {
                // Build a description for it
                MCacheFormatDescription::CacheDataType mayaDataType = _sdfValueTypeNameToMCacheDataType(usdAttrType);
                if (mayaDataType == MCacheFormatDescription::kUnknownData) {
                    TF_WARN("Unknown or unsupported attribute type for %s", usdAttrName.GetText());
                    return cachedAttributes.end();
                }
                attrDef.mayaAttrName = usdAttrName;
                attrDef.mayaDataType = mayaDataType;
                attrDef.usdAttrName = usdAttrName;
                attrDef.usdAttrType = usdAttrType;
            }
            // No need to add it on the default prim the goal of this method
            // is to help looking for maya channel / attributes corresponding
            // to existing attributes in a usd cache
            cachedAttributes.push_back(attrDef);
            foundIt = std::prev(cachedAttributes.end());
        }

        return foundIt;
    }
};

TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaCacheFormatTokens,
                        PXRUSDMAYA_CACHEFORMAT_TOKENS);

UsdCacheFormat::UsdCacheFormat() : impl(new Impl), mWriteTimeCalledOnce(false) {
}

UsdCacheFormat::~UsdCacheFormat() {
}

MString UsdCacheFormat::extension() {
    // For filtering display of files on disk
    return MString(PxrUsdMayaCacheFormatTokens->UsdFileExtensionDefault.GetText());
}

bool UsdCacheFormat::handlesDescription() {
    return false;
}

MStatus UsdCacheFormat::open(const MString& fileName, FileAccessMode mode) {
    if (fileName.length() == 0) {
        TF_WARN("Empty filename!");
        return MS::kFailure;
    }
    std::string iFileName = fileName.asChar();
    auto iFileBase = TfStringGetBeforeSuffix(iFileName, '.');
    auto iFileExtension = TfStringGetSuffix(iFileName, '.');
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
                    || (!isValid())) {
                closeLayer();
            }
        }
        if (!mLayerPtr) {
            // If we are overwriting a file that already has been loaded
            // should we trash the layer or edit it?
            mLayerPtr = SdfLayer::Find(iFileName);
            if (mLayerPtr && !isValid()) {
                closeLayer();
            }
        }
        if (!mLayerPtr) {
            mLayerPtr = SdfLayer::CreateNew(iFileName);
        }
        if (mLayerPtr) {
            mLayerPtr->SetPermissionToEdit(true);
            mLayerPtr->SetPermissionToSave(true);
            return MS::kSuccess;
        } else {
            TF_WARN("(write) could not create a layer to %s", iFileName.c_str());
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
                closeLayer();
            }
        }
        if (!mLayerPtr) {
            mLayerPtr = SdfLayer::FindOrOpen(iFileName);
        }
        if (mLayerPtr) {
            if (readHeader()) {
                mLayerPtr->SetPermissionToEdit(false);
                mLayerPtr->SetPermissionToSave(false);
                return MS::kSuccess;
            } else {
                TF_WARN("(read) invalid cache format %s", iFileName.c_str());
                return MS::kFailure;
            }
        } else {
            TF_WARN("(read) could not read a layer from %s", iFileName.c_str());
            return MS::kFailure;
        }
    } else {
        TF_WARN("Append (read + write) file open mode unsupported");
        return MS::kFailure;
    }
}

void UsdCacheFormat::closeLayer()
{
    if (mLayerPtr) {
        impl->cachedAttributes.clear();
        mLayerPtr.Reset();
    }
}

void UsdCacheFormat::close()
{
    // Maya can call close file directly but we need to control when we dump a layer if ever
}

MStatus UsdCacheFormat::isValid() {
    // Checking that it has a correct header is done just once at open
    // since Maya seems to be calling it a lot, kept it minimal
    if (mLayerPtr) {
        return MS::kSuccess;
    } else {
        return MS::kFailure;
    }
}

MStatus UsdCacheFormat::writeHeader(const MString& version,
        MTime& startTime,
        MTime& endTime) {
    if (mLayerPtr) {
        mLayerPtr->SetComment(translatorName
                              + std::string(" version ")
                              + version.asChar());
        // When writeHeader is called, we get a startTime of 0 and endTime of 1 ?
        // These will be update at each call to writeTime anyway
        mLayerPtr->SetStartTimeCode(startTime.asUnits(_timeUnit));
        mLayerPtr->SetEndTimeCode(endTime.asUnits(_timeUnit));
        mCurrentTime = startTime.asUnits(_timeUnit);
        // This will not change and can be done once and for all here
        MTime oneSecond(1, MTime::kSeconds);
        const auto framesPerSecond = oneSecond.asUnits(MTime::uiUnit());
        const auto timesCodesPerSecond = oneSecond.asUnits(_timeUnit);
        const auto timeCodesPerFrame = timesCodesPerSecond / framesPerSecond;
        mLayerPtr->SetFramesPerSecond(framesPerSecond);
        mLayerPtr->SetTimeCodesPerSecond(timesCodesPerSecond);
        mLayerPtr->SetFramePrecision(timeCodesPerFrame);
        // Create the default primitive
        const SdfPath specPath = _findOrAddDefaultPrim(mLayerPtr);
        if (specPath.IsPrimPath()) {
            return MS::kSuccess;
        } else {
            TF_WARN("Could not find or create the cache default prim.");
            return MS::kFailure;
        }
    } else {
        TF_WARN("Got called with no open layer.");
        return MS::kFailure;
    }
}

MStatus UsdCacheFormat::readHeader() {
    if (mLayerPtr) {
        const auto comment = mLayerPtr->GetComment();
        if (TfStringStartsWith(comment, translatorName)) {
            // If it looks like a proper cache file, read the attributes declared
            // on the default prim
            return readExistingAttributes();
        }
    }
    return MS::kFailure;
}

MStatus UsdCacheFormat::rewind() {
    if (mLayerPtr) {
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

MStatus UsdCacheFormat::readExistingAttributes()
{
    // Get the existing attributes on the layer default prim
    // and add them to the set of cached attributes
    MStatus status = MS::kSuccess;
    impl->cachedAttributes.clear();
    SdfPath primPath(mLayerPtr->GetDefaultPrim());
    SdfPrimSpecHandle primSpec = mLayerPtr->GetPrimAtPath(primPath);
    for (const auto& attr: primSpec->GetProperties()) {
        SdfPath usdAttrPath = attr->GetPath();
        TfToken usdAttrName = attr->GetNameToken();
        SdfValueTypeName usdAttrType = attr->GetTypeName();
        // Add it to the set of cached attributes
        const auto it = impl->findOrAddUsdAttribute(usdAttrName, usdAttrType);
        if (it != impl->cachedAttributes.end()) {
            // TODO: figure out what needs to be added here.
        } else {
            TF_WARN("Could not add %s", usdAttrName.GetString().c_str());
            status = MS::kFailure;
        }
    }
    if (!impl->cachedAttributes.empty()) {
        return status;
    } else {
        TF_WARN("Found no attributes on default prim %s", primPath.GetString().c_str());
        return MS::kFailure;
    }
}

MStatus UsdCacheFormat::writeTime(MTime& time)
{
    mCurrentTime = time.asUnits(_timeUnit);
    if (mWriteTimeCalledOnce) {
        if ((!mLayerPtr->HasStartTimeCode())
            || (mCurrentTime < mLayerPtr->GetStartTimeCode())) {
            mLayerPtr->SetStartTimeCode(mCurrentTime);
        }
        if ((!mLayerPtr->HasEndTimeCode())
            || (mCurrentTime > mLayerPtr->GetEndTimeCode())) {
            mLayerPtr->SetEndTimeCode(mCurrentTime);
        }
    } else {
        mLayerPtr->SetStartTimeCode(mCurrentTime);
        mLayerPtr->SetEndTimeCode(mCurrentTime);
        mWriteTimeCalledOnce = true;
    }
    return MS::kSuccess;
}

MStatus UsdCacheFormat::readTime(MTime& time) {
    time = MTime(mCurrentTime, _timeUnit);
    return MS::kSuccess;
}

MStatus UsdCacheFormat::findTime(MTime& time, MTime& foundTime) {
    // It the Maya dev kit examples it states
    // "Find the biggest cached time, which is smaller or equal to seekTime and return foundTime"
    const auto seekTime = time.asUnits(_timeUnit);
    auto lowerTime = std::numeric_limits<double>::infinity();
    auto upperTime = std::numeric_limits<double>::infinity();
    mLayerPtr->GetBracketingTimeSamples(seekTime, &lowerTime, &upperTime);
    if (lowerTime <= seekTime) {
        foundTime = MTime(lowerTime, _timeUnit);
        mCurrentTime = lowerTime;
        return MS::kSuccess;
    } else {
        // rewind();
        return MS::kFailure;
    }
}

MStatus UsdCacheFormat::readNextTime(MTime& foundTime) {
    // It's 1 at 6000 fps here. Maya seems to only use this setting
    // when communicating with cache, so it's 1/250th of a frame
    // at actual 24fps. Is it small enough for our needs, it must be small
    // enough not to overshoot a possible sub frame sample.
    // Or use UsdTimeCode SafeStep or calculate it from the SdfLayer
    // FramePrecision field ?
    constexpr double epsilonTime = 1.0;
    const auto seekTime = mCurrentTime + epsilonTime;
    auto lowerTime = mCurrentTime;
    auto upperTime = mCurrentTime;
    if (mLayerPtr->GetBracketingTimeSamples(seekTime,
            &lowerTime, &upperTime)) {
        if ((upperTime < std::numeric_limits<double>::infinity())
                && (upperTime >= mCurrentTime + epsilonTime)) {
            mCurrentTime = upperTime;
            foundTime = MTime(upperTime, _timeUnit);
            return MS::kSuccess;
        }
    }
    return MS::kFailure;
}

MStatus UsdCacheFormat::writeChannelName(const MString& name) {
    // Actually adding the attribute will happen later when we know
    // which type Maya is trying to write
    mCurrentChannel = std::string(name.asChar());
    return MS::kSuccess;
}

MStatus UsdCacheFormat::findChannelName(const MString& name) {
    // All channels for kRead should already have been added because
    // Maya will call readChannelName immediatly after calling
    // findChannelName on first channel in set
    const std::string channel = name.asChar();
    const auto mayaAttrName = _mayaAttrFromChannel(channel);
    const auto itName = impl->cachedAttributes.get<_Maya>().find(mayaAttrName);
    if (itName != impl->cachedAttributes.get<_Maya>().end()) {
        impl->firstMayaAttr = impl->cachedAttributes.project<0>(itName);
        mCurrentChannel = channel;
        return MS::kSuccess;
    } else {
        mCurrentChannel = "";
        TF_WARN("Error finding channel %s", name.asChar());
        return MS::kFailure;
    }
}

MStatus UsdCacheFormat::readChannelName(MString& name) {
    // It's actually a find next channel name for Maya, caller is using the MS::kFailure
    // return not as an actual error but as an indication we read last channel
    const auto mayaNodeName = _mayaNodeFromChannel(mCurrentChannel);
    const auto mayaAttrName = _mayaAttrFromChannel(mCurrentChannel);
    const auto itName = impl->cachedAttributes.get<_Maya>().find(mayaAttrName);
    if (itName != impl->cachedAttributes.get<_Maya>().end()) {
        auto itId = impl->cachedAttributes.project<0>(itName);
        // We keep track of first requested attribute with mFirstMayaAttr
        // and loop on the list so that we are not dependant on Maya order of attributes
        if (++itId ==  impl->cachedAttributes.end()) {
            itId = impl->cachedAttributes.begin();
        }
        // Just remove the above and instead check :
        // if (++itId !=  mCachedAttributes.end()) if we don't want to loop
        if (itId != impl->firstMayaAttr) {
            const std::string nextMayaAttrName = itId->mayaAttrName;
            const std::string nextChannel = TfStringPrintf("%s_%s", mayaNodeName.c_str(), nextMayaAttrName.c_str());
            // Advance by one channel
            mCurrentChannel = nextChannel;
            name = nextChannel.c_str();
            return MS::kSuccess;
        }
    }
    // No more channels
    // mCurrentChannel = "";
    name = MString("");
    return MS::kFailure;
}

void UsdCacheFormat::beginWriteChunk() {
    // Nothing to do
}

void UsdCacheFormat::endWriteChunk() {
    // Save each frame
    mLayerPtr->Save();
}

MStatus UsdCacheFormat::beginReadChunk() {
    // Nothing to do
    return MS::kSuccess;
}

void UsdCacheFormat::endReadChunk() {
    // Nothing to do
}

MStatus UsdCacheFormat::writeInt32(int i)
{
    // Try to add on the fly for writing first frame
    // Note: there is no MCacheFormatDescription for int32 and it seems never to
    // be called by Maya anyway
    const auto it = impl->findOrAddMayaAttribute(
        mLayerPtr,
        _mayaAttrFromChannel(mCurrentChannel),
        MCacheFormatDescription::kDouble);
    if (it != impl->cachedAttributes.end()) {
        if (it->convertToUsd == nullptr) {
            mLayerPtr->SetTimeSample(_usdPathFromAttribute(mLayerPtr, it->usdAttrName),
                                     mCurrentTime,
                                     i);
        } else {
            mLayerPtr->SetTimeSample(_usdPathFromAttribute(mLayerPtr, it->usdAttrName),
                                     mCurrentTime,
                                     it->convertToUsd(VtValue(i)));
        }
        return MS::kSuccess;
    } else {
        TF_WARN("Error writing integer %i to %s", i, mCurrentChannel.c_str());
        return MS::kFailure;
    }
}

int UsdCacheFormat::readInt32() {
    int result = 0;
    const auto it = impl->findOrAddMayaAttribute(
        mLayerPtr,
        _mayaAttrFromChannel(mCurrentChannel),
        MCacheFormatDescription::kDouble);
    if (it != impl->cachedAttributes.end()) {
        VtValue value(result);
        mLayerPtr->QueryTimeSample(_usdPathFromAttribute(mLayerPtr, it->usdAttrName),
                                   mCurrentTime,
                                   &value);
        if (it->convertFromUsd == nullptr) {
            result = value.UncheckedGet<int>();
        } else {
            result = it->convertFromUsd(value).UncheckedGet<int>();
        }
    }
    return result;
}

MStatus UsdCacheFormat::writeDoubleArray(const MDoubleArray& array) {
    const auto it = impl->findOrAddMayaAttribute(
        mLayerPtr,
        _mayaAttrFromChannel(mCurrentChannel),
        MCacheFormatDescription::kDoubleArray);
    if (it != impl->cachedAttributes.end()) {
        const SdfPath attrPath = _usdPathFromAttribute(mLayerPtr, it->usdAttrName);
        const SdfValueTypeName usdAttrType = it->usdAttrType;
        if ((SdfValueTypeNames->DoubleArray == usdAttrType)
            && (it->convertToUsd == nullptr)) {
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
                                     _convertOutArray<MDoubleArray, double>(array, it->convertToUsd));
        } else if (SdfValueTypeNames->FloatArray == usdAttrType) {
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     _convertOutArray<MDoubleArray, float>(array, it->convertToUsd));
        } else if (SdfValueTypeNames->Int64Array == usdAttrType) {
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     _convertOutArray<MDoubleArray, long>(array, it->convertToUsd));
        } else if (SdfValueTypeNames->UInt64Array == usdAttrType) {
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     _convertOutArray<MDoubleArray, unsigned long>(array, it->convertToUsd));
        } else {
            TF_WARN("Incorrect SdfType for double array %s , %s",
                    mCurrentChannel.c_str(), usdAttrType.GetAsToken().GetText());
            return MS::kFailure;
        }
        return MS::kSuccess;
    } else {
        TF_WARN("Error writing double array to %s", mCurrentChannel.c_str());
        return MS::kFailure;
    }
}

MStatus UsdCacheFormat::writeFloatArray(const MFloatArray& array) {
    const auto it = impl->findOrAddMayaAttribute(
        mLayerPtr,
        _mayaAttrFromChannel(mCurrentChannel),
        MCacheFormatDescription::kFloatArray);
    if (it != impl->cachedAttributes.end()) {
        const SdfPath attrPath = _usdPathFromAttribute(mLayerPtr, it->usdAttrName);
        const SdfValueTypeName usdAttrType = it->usdAttrType;
        if ((SdfValueTypeNames->FloatArray == usdAttrType)
            && (it->convertToUsd == nullptr)) {
            VtFloatArray varray(array.length());
            array.get(varray.data());
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     varray);
        } else if (SdfValueTypeNames->DoubleArray == usdAttrType) {
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     _convertOutArray<MFloatArray, double>(array, it->convertToUsd));
        } else if (SdfValueTypeNames->FloatArray == usdAttrType) {
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     _convertOutArray<MFloatArray, float>(array, it->convertToUsd));
        } else if (SdfValueTypeNames->Int64Array == usdAttrType) {
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     _convertOutArray<MFloatArray, long>(array, it->convertToUsd));
        } else if (SdfValueTypeNames->UInt64Array == usdAttrType) {
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     _convertOutArray<MFloatArray, unsigned long>(array, it->convertToUsd));
        } else {
            TF_WARN("Incorrect SdfType for float array %s , %s",
                    mCurrentChannel.c_str(), usdAttrType.GetAsToken().GetText());
            return MS::kFailure;
        }
        return MS::kSuccess;
    } else {
        TF_WARN("Error writing float array to %s", mCurrentChannel.c_str());
        return MS::kFailure;
    }
}

MStatus UsdCacheFormat::writeDoubleVectorArray(const MVectorArray& array) {
    const auto it = impl->findOrAddMayaAttribute(
        mLayerPtr,
        _mayaAttrFromChannel(mCurrentChannel),
        MCacheFormatDescription::kDoubleVectorArray);
    if (it != impl->cachedAttributes.end()) {
        const SdfPath attrPath = _usdPathFromAttribute(mLayerPtr, it->usdAttrName);
        const SdfValueTypeName usdAttrType = it->usdAttrType;
        if ((SdfValueTypeNames->Vector3dArray == usdAttrType)
            || (SdfValueTypeNames->Point3dArray == usdAttrType)
            || (SdfValueTypeNames->Normal3dArray == usdAttrType)
            || (SdfValueTypeNames->Color3dArray == usdAttrType)) {
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     _convertOutVectorArray<MVectorArray, GfVec3d>(array, it->convertToUsd));
        } else if ((SdfValueTypeNames->Vector3fArray == usdAttrType)
                   || (SdfValueTypeNames->Point3fArray == usdAttrType)
                   || (SdfValueTypeNames->Normal3fArray == usdAttrType)
                   || (SdfValueTypeNames->Color3fArray == usdAttrType)) {
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     _convertOutVectorArray<MVectorArray, GfVec3f>(array, it->convertToUsd));
        } else {
            TF_WARN("Incorrect SdfType for double vector array %s , %s",
                    mCurrentChannel.c_str(), usdAttrType.GetAsToken().GetText());
            return MS::kFailure;
        }
        return MS::kSuccess;
    } else {
        TF_WARN("Error writing double vector array to %s", mCurrentChannel.c_str());
        return MS::kFailure;
    }
}

MStatus UsdCacheFormat::writeFloatVectorArray(const MFloatVectorArray& array) {
    const auto it = impl->findOrAddMayaAttribute(
        mLayerPtr,
        _mayaAttrFromChannel(mCurrentChannel),
        MCacheFormatDescription::kFloatVectorArray);
    if (it != impl->cachedAttributes.end()) {
        const SdfPath attrPath = _usdPathFromAttribute(mLayerPtr, it->usdAttrName);
        const SdfValueTypeName usdAttrType = it->usdAttrType;
        if ((SdfValueTypeNames->Vector3dArray == usdAttrType)
            || (SdfValueTypeNames->Point3dArray == usdAttrType)
            || (SdfValueTypeNames->Normal3dArray == usdAttrType)
            || (SdfValueTypeNames->Color3dArray == usdAttrType)) {
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     _convertOutVectorArray<MFloatVectorArray, GfVec3d>(array, it->convertToUsd));
        } else if ((SdfValueTypeNames->Vector3fArray == usdAttrType)
                   || (SdfValueTypeNames->Point3fArray == usdAttrType)
                   || (SdfValueTypeNames->Normal3fArray == usdAttrType)
                   || (SdfValueTypeNames->Color3fArray == usdAttrType)) {
            mLayerPtr->SetTimeSample(attrPath,
                                     mCurrentTime,
                                     _convertOutVectorArray<MFloatVectorArray, GfVec3f>(array, it->convertToUsd));
        } else {
            TF_WARN("Incorrect SdfType for float array %s , %s",
                    mCurrentChannel.c_str(), usdAttrType.GetAsToken().GetText());
            return MS::kFailure;
        }
        return MS::kSuccess;
    } else {
        TF_WARN("Error writing float vector array to %s", mCurrentChannel.c_str());
        return MS::kFailure;
    }
}

unsigned UsdCacheFormat::readArraySize() {
    // Might be able to pass back a default size && wait until the actual read for resizing
    // All attributes should exist already on the defaultPrim since we're doing a read
    unsigned result = 0;
    const auto mayaAttrName = _mayaAttrFromChannel(mCurrentChannel);
    const auto it = impl->cachedAttributes.get<_Maya>().find(mayaAttrName);
    if (it != impl->cachedAttributes.get<_Maya>().end()) {
        TfToken usdAttrName = it->usdAttrName;
        SdfValueTypeName usdAttrType = it->usdAttrType;
        SdfPath attrPath = _usdPathFromAttribute(mLayerPtr, usdAttrName);
        if (usdAttrType) {
            if (usdAttrType.IsArray()) {
                VtValue varray;
                if (mLayerPtr->QueryTimeSample(attrPath, mCurrentTime, &varray)) {
                    result = varray.GetArraySize();
                }
            } else {
                result = 1;
            }
        } else {
            TF_WARN("Unsupported type for attribute %s", usdAttrName.GetText());
        }
    } else {
        TF_WARN("Cannot find attribute for channel %s", mCurrentChannel.c_str());
    }
    return result;
}

MStatus UsdCacheFormat::readDoubleArray(MDoubleArray& array,
        unsigned arraySize)
{
    const auto mayaAttrName = _mayaAttrFromChannel(mCurrentChannel);
    const auto it = impl->cachedAttributes.get<_Maya>().find(mayaAttrName);
    if (it != impl->cachedAttributes.get<_Maya>().end()) {
        auto attrPath = _usdPathFromAttribute(mLayerPtr, it->usdAttrName);
        VtValue value;
        if (mLayerPtr->QueryTimeSample(attrPath,
                                       mCurrentTime,
                                       &value)) {
            SdfValueTypeName usdAttrType = it->usdAttrType;
            if (SdfValueTypeNames->DoubleArray == usdAttrType) {
                _convertInArray<double, MDoubleArray>(value, array, it->convertFromUsd);
            } else if (SdfValueTypeNames->FloatArray == usdAttrType) {
                _convertInArray<float, MDoubleArray>(value, array, it->convertFromUsd);
            } else if (SdfValueTypeNames->Int64Array == usdAttrType) {
                _convertInArray<long, MDoubleArray>(value, array, it->convertFromUsd);
            } else if (SdfValueTypeNames->UInt64Array == usdAttrType) {
                _convertInArray<unsigned long, MDoubleArray>(value, array, it->convertFromUsd);
            } else {
                return MS::kFailure;
            }
            if (array.length() == arraySize) {
                return MS::kSuccess;
            } else {
                TF_WARN("Incorrect array length wheb reading %u <> %u", array.length(), arraySize);
            }
        }
    }
    return MS::kFailure;
}

MStatus UsdCacheFormat::readFloatArray(MFloatArray& array,
        unsigned arraySize) {
    const auto mayaAttrName = _mayaAttrFromChannel(mCurrentChannel);
    const auto it = impl->cachedAttributes.get<_Maya>().find(mayaAttrName);
    if (it != impl->cachedAttributes.get<_Maya>().end()) {
        SdfPath attrPath = _usdPathFromAttribute(mLayerPtr, it->usdAttrName);
        VtValue value;
        if (mLayerPtr->QueryTimeSample(attrPath,
                                       mCurrentTime,
                                       &value)) {
            SdfValueTypeName usdAttrType = it->usdAttrType;
            if (SdfValueTypeNames->DoubleArray == usdAttrType) {
                _convertInArray<double, MFloatArray>(value, array, it->convertFromUsd);
            } else if (SdfValueTypeNames->FloatArray == usdAttrType) {
                _convertInArray<float, MFloatArray>(value, array, it->convertFromUsd);
            } else if (SdfValueTypeNames->Int64Array == usdAttrType) {
                _convertInArray<long, MFloatArray>(value, array, it->convertFromUsd);
            } else if (SdfValueTypeNames->UInt64Array == usdAttrType) {
                _convertInArray<unsigned long, MFloatArray>(value, array, it->convertFromUsd);
            } else {
                return MS::kFailure;
            }
            if (array.length() == arraySize) {
                return MS::kSuccess;
            } else {
                TF_WARN("Incorrect array length wheb reading %u <> %u", array.length(), arraySize);
            }
        }
    }
    return MS::kFailure;
}

MStatus UsdCacheFormat::readDoubleVectorArray(MVectorArray& array,
        unsigned arraySize) {
    const std::string mayaAttrName = _mayaAttrFromChannel(mCurrentChannel);
    const auto it = impl->cachedAttributes.get<_Maya>().find(mayaAttrName);
    if (it != impl->cachedAttributes.get<_Maya>().end()) {
        SdfPath attrPath = _usdPathFromAttribute(mLayerPtr, it->usdAttrName);
        VtValue value;
        if (mLayerPtr->QueryTimeSample(attrPath,
                                       mCurrentTime,
                                       &value)) {
            SdfValueTypeName usdAttrType = it->usdAttrType;
            if ((SdfValueTypeNames->Vector3dArray == usdAttrType)
                || (SdfValueTypeNames->Point3dArray == usdAttrType)
                || (SdfValueTypeNames->Normal3dArray == usdAttrType)
                || (SdfValueTypeNames->Color3dArray == usdAttrType)) {
                _convertInVectorArray<GfVec3d, MVectorArray>(value, array, it->convertFromUsd);
            } else if ((SdfValueTypeNames->Vector3fArray == usdAttrType)
                       || (SdfValueTypeNames->Point3fArray == usdAttrType)
                       || (SdfValueTypeNames->Normal3fArray == usdAttrType)
                       || (SdfValueTypeNames->Color3fArray == usdAttrType)) {
                _convertInVectorArray<GfVec3f, MVectorArray>(value, array, it->convertFromUsd);
            } else {
                return MS::kFailure;
            }
            if (array.length() == arraySize) {
                return MS::kSuccess;
            } else {
                TF_WARN("Incorrect array length wheb reading %u <> %u", array.length(), arraySize);
            }
        }
    }
    return MS::kFailure;
}

MStatus UsdCacheFormat::readFloatVectorArray(MFloatVectorArray& array,
        unsigned arraySize) {
    const auto mayaAttrName = _mayaAttrFromChannel(mCurrentChannel);
    const auto it = impl->cachedAttributes.get<_Maya>().find(mayaAttrName);
    if (it != impl->cachedAttributes.get<_Maya>().end()) {
        SdfPath attrPath = _usdPathFromAttribute(mLayerPtr, it->usdAttrName);
        VtValue value;
        if (mLayerPtr->QueryTimeSample(attrPath,
                                       mCurrentTime,
                                       &value)) {
            SdfValueTypeName usdAttrType = it->usdAttrType;
            if ((SdfValueTypeNames->Vector3dArray == usdAttrType)
                || (SdfValueTypeNames->Point3dArray == usdAttrType)
                || (SdfValueTypeNames->Normal3dArray == usdAttrType)
                || (SdfValueTypeNames->Color3dArray == usdAttrType)) {
                _convertInVectorArray<GfVec3d, MFloatVectorArray>(value, array, it->convertFromUsd);
            } else if ((SdfValueTypeNames->Vector3fArray == usdAttrType)
                       || (SdfValueTypeNames->Point3fArray == usdAttrType)
                       || (SdfValueTypeNames->Normal3fArray == usdAttrType)
                       || (SdfValueTypeNames->Color3fArray == usdAttrType)) {
                _convertInVectorArray<GfVec3f, MFloatVectorArray>(value, array, it->convertFromUsd);
            } else {
                return MS::kFailure;
            }
            if (array.length() == arraySize) {
                return MS::kSuccess;
            } else {
                TF_WARN("Incorrect array length wheb reading %u <> %u", array.length(), arraySize);
            }
        }
    }
    return MS::kFailure;
}

PXR_NAMESPACE_CLOSE_SCOPE
