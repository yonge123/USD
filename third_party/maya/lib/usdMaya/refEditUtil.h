//
// Created by nickk on 1/31/17.
//

#ifndef USD_REFEDITUTIL_H
#define USD_REFEDITUTIL_H


#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/fileUtils.h"
#include <maya/MObject.h>

#include "usdMaya/util.h"

#include <string>
#include <vector>
#include <map>

/// Object that holds information needed for MayaPrimWriter to know whether
/// to write out an attribute
struct RefEdits{
    RefEdits();

    TfHashSet<std::string, TfHash> modifiedAttrs;
    bool isReferenced;
};

/// Class to store reference edits of assemblies or references
class RefEditUtil {
public:
    RefEditUtil(const bool mergeTransformAndShape);
    ~RefEditUtil();

    /// Get the refEdits that correspond to a specific dagNode
    ///
    /// Returns false if node is referenced and no edits were found.
    bool GetDagNodeEdits(const MDagPath& dagPath, RefEdits& refEdits);

    /// Collect the refEdits of a reference or assembly
    void ProcessReference(const MObject& referenceObj);

private:

    using SdfPathHashSetMap = std::unordered_map<SdfPath, TfHashSet<std::string, TfHash>, SdfPath::Hash>;

    SdfPathHashSetMap mPrimPathToRefEdits;

    TfHashSet<std::string, TfHash> mReferences;

    bool mMergeTransformAndShape;

};


#endif //USD_REFEDITUTIL_H
