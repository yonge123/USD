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
#ifndef PXRUSDMAYA_JOBARGS_H
#define PXRUSDMAYA_JOBARGS_H

/// \file JobArgs.h

#include "usdMaya/util.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usd/usdFileFormat.h"
#include "pxr/usd/usd/usdaFileFormat.h"
#include "pxr/usd/usd/usdcFileFormat.h"

#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>


#define PXRUSDMAYA_TRANSLATOR_TOKENS \
    ((UsdFileExtensionDefault, \
        SdfFileFormat::FindById(UsdUsdFileFormatTokens->Id)->GetPrimaryFileExtension())) \
    ((UsdFileFilter, \
        std::string("*.") \
            + SdfFileFormat::FindById(UsdUsdFileFormatTokens->Id)->GetPrimaryFileExtension() \
            + " *." \
            + SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->GetPrimaryFileExtension() \
            + " *." \
            + SdfFileFormat::FindById(UsdUsdcFileFormatTokens->Id)->GetPrimaryFileExtension()))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaTranslatorTokens,
        PXRUSDMAYA_TRANSLATOR_TOKENS);

#define PXRUSDMAYA_JOBARGS_TOKENS \
    (Full) \
    (Collapsed) \
    (Uniform) \
    (defaultLayer) \
    (currentLayer) \
    (modelingVariant)

TF_DECLARE_PUBLIC_TOKENS(PxUsdExportJobArgsTokens, 
        PXRUSDMAYA_JOBARGS_TOKENS);

struct JobSharedArgs
{
    JobSharedArgs();

    // Templating since we know at compile time which subclass we have
    template<typename JobArgClass>
    void parseOptionsString(const MString &optionsString)
    {
        if ( optionsString.length() > 0 ) {
            MStringArray optionList;
            MStringArray theOption;
            optionsString.split(';', optionList);
            for (unsigned int i = 0; i < optionList.length(); ++i) {
                theOption.clear();
                optionList[i].split('=', theOption);
                static_cast<JobArgClass*>(this)->parseSingleOption(theOption);
            }
        }
    }

    bool parseSharedOption(const MStringArray& theOption);

    void setDefaultMeshScheme(const MString& stringVal);

    std::string fileName;
    TfToken shadingMode;
    TfToken defaultMeshScheme;
    double startTime;
    double endTime;

};

struct JobExportArgs : JobSharedArgs
{
    JobExportArgs();

    inline void parseExportOptions(const MString &optionsString)
    {
        parseOptionsString<JobExportArgs>(optionsString);
    }

    void parseSingleOption(const MStringArray& theOption);

    std::set<double> frameSamples;

    bool exportRefsAsInstanceable;
    bool exportDisplayColor;

    bool mergeTransformAndShape;

    bool exportAnimation;
    bool excludeInvisible;
    bool exportDefaultCameras;

    bool exportMeshUVs;
    bool normalizeMeshUVs;

    bool normalizeNurbs;
    bool exportNurbsExplicitUV;
    TfToken nurbsExplicitUVType;

    bool exportColorSets;

    TfToken renderLayerMode;

    bool exportVisibility;

    std::string melPerFrameCallback;
    std::string melPostCallback;
    std::string pythonPerFrameCallback;
    std::string pythonPostCallback;

    PxrUsdMayaUtil::ShapeSet dagPaths;

    std::vector<std::string> chaserNames;
    typedef std::map<std::string, std::string> ChaserArgs;
    std::map< std::string, ChaserArgs > allChaserArgs;

    // This path is provided when dealing with variants
    // where a _BaseModel_ root path is used instead of
    // the model path. This to allow a proper internal reference
    SdfPath usdModelRootOverridePath;

    // Optionally specified path to use as top level prim in
    // place of the scene root.
    std::string exportRootPath;
    // store a computed SdfPath path for reuse in mayaPrimWriter
    SdfPath exportRootSdfPath;

    TfToken rootKind;

    // Whether to try to handle namespaces added by usd references / assemblies,
    // so that usd paths on export match the original usd paths
    bool handleUsdNamespaces;
};

struct JobImportArgs : JobSharedArgs
{
    JobImportArgs();

    inline void parseImportOptions(const MString &optionsString)
    {
        parseOptionsString<JobImportArgs>(optionsString);
    }

    void parseSingleOption(const MStringArray& theOption);

    void setJoinedParentRefPaths(const std::string& joinedRefPaths);

    std::string primPath;
    TfToken assemblyRep;
    bool readAnimData;
    bool useCustomFrameRange;
    bool importWithProxyShapes;
    // If true, will use maya assemblies for usd sub-references; if false, will
    // use maya references
    bool useAssemblies;
    std::string parentNode;
    std::vector<std::string> parentRefPaths;
};


#endif // PXRUSDMAYA_JOBARGS_H
