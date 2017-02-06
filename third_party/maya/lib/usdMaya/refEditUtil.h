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

    bool isReferenced;
    // TfHashSet?
    TfHashSet<std::string, TfHash> modifiedAttrs;
};

// class to store reference edits of assemblies
class RefEditUtil {
public:
    RefEditUtil(const bool mergeTransformAndShape, const bool stripUsdNamespaces);
    ~RefEditUtil();

    /// Get the refEdits that correspond to a specific dagNode
    void GetDagNodeEdits(const MDagPath& dagPath, RefEdits& refEdits);

    /// Collect the refEdits of a reference or assembly
    void ProcessReference(const MObject&);

private:

    typedef std::unordered_map<SdfPath, TfHashSet<std::string, TfHash>, SdfPath::Hash> SdfPathHashSetMap;

    bool _mergeTransformAndShape;
    bool _stripUsdNamespaces;

    TfHashSet<std::string, TfHash> _references;
    SdfPathHashSetMap _primPathToRefEdits;

};


#endif //USD_REFEDITUTIL_H
