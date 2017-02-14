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
#ifndef PXRUSDMAYA_USDREADJOB_H
#define PXRUSDMAYA_USDREADJOB_H

/// \file usdReadJob.h

#include "pxr/pxr.h"
#include "usdMaya/JobArgs.h"
#include "usdMaya/primReaderContext.h"
#include "usdMaya/variantSelectionNode.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/treeIterator.h"

#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE



class usdReadJob
{
  public:
    usdReadJob(const std::map<std::string, std::string>& topVariants,
        const JobImportArgs & iArgs,

        // We need to know the names of the assembly and proxy shape types for
        // when we need to create them. This is specified in the creator for
        // the command that uses this class. See usdImport.
        const std::string& assemblyTypeName,
        const std::string& proxyShapeTypeName);

    ~usdReadJob();

    bool doIt(std::vector<MDagPath>* addedDagPaths);
    bool redoIt();
    bool undoIt();

    // Getters/Setters
    void setMayaRootDagPath(const MDagPath &mayaRootDagPath) { mMayaRootDagPath = mayaRootDagPath; };

  private:
    bool _InitVariantsByPath(const std::map<std::string, std::string>& topVariants);

    // XXX: Activating the 'Expanded' representation of a USD reference
    // assembly node is very much like performing a regular usdReadJob but with
    // a few key differences (e.g. creating proxy shapes at collapse points).
    // These private helper methods cover the functionality of a regular
    // usdImport, and an 'Expanded' representation-style import, respectively.
    // It would be great if we could combine these into a single traversal at
    // some point.
    bool _DoImport(UsdTreeIterator& primIt,
                   const UsdPrim& usdRootPrim);
    bool _DoImportWithProxies(UsdTreeIterator& primIt);

    // These are helper methods for the proxy import method.
    bool _ProcessProxyPrims(
        const std::vector<UsdPrim>& proxyPrims,
        const UsdPrim& pxrGeomRoot,
        const std::vector<std::string>& collapsePointPathStrings);
    bool _ProcessSubAssemblyPrims(const std::vector<UsdPrim>& subAssemblyPrims);
    bool _ProcessCameraPrims(const std::vector<UsdPrim>& cameraPrims);

    // Data
    JobImportArgs mArgs;

    // TODO: allow the class to hold just a reference (perhaps a smart
    // pointer?), as these maps could get large, and we may want to avoid
    // copying unless necessary.

    // Variant selections, by usd path. An empty string can be used to indicate
    // the variant selection for the root layer prim (ie, whatever
    // UsdUtilsGetModelNameFromRootLayer resolves to). If, after resolving the
    // root prim, it is the same as an entry already in the mVariantsByPath,
    // the two variant-maps will be merged, but the ones stored under the
    // "empty string" will take precedence.
    //
    // Ie, say mVariantsByPath looks like this:
    //
    // mVariantsByPath = {
    //     "": {"color": "black", "detail": "high"},
    //     "/World/superDude": {"color": "red", "utilityBelt": "combat"},
    // }
    //
    // ...and the mArgs.primPath resolves to "/World/superDude". Then the final
    // merged variant-selections that will be used for "/World/superDude" are:
    //
    // {"color": "black", "detail": "high", "utilityBelt": "combat"}
    //
    // Note that "color" is "black", since the entries for the empty string take
    // precedence.
    VariantMapByPath mVariantsByPath;

    MDagModifier mDagModifierUndo;
    bool mDagModifierSeeded;
    typedef PxrUsdMayaPrimReaderContext::ObjectRegistry PathNodeMap;
    PathNodeMap mNewNodeRegistry;
    MDagPath mMayaRootDagPath;

    const std::string _assemblyTypeName;
    const std::string _proxyShapeTypeName;
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_USDREADJOB_H
