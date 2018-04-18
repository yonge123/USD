//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_SKELBINDINGSWRITER_H
#define PXRUSDMAYA_SKELBINDINGSWRITER_H

/// \file skelBindingsWriter.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathTable.h"
#include "usdMaya/util.h"

#include <maya/MDagPath.h>

#include <set>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE


/// This class encapsulates all of the logic for writing skel bindings at the
/// appropriate points under all the SkelRoots in a stage.
/// The class generates final skel bindings by "moving" the skel binding sites
/// upward in the hierarchy to reduce the number of skel:skeleton rels that
/// need to be authored. For example, if /A/B/X and /A/B/Y are both bound to
/// /A/MyChar_skeleton, and there are no other prims in /A/B's subtree that are
/// skeletally bound, then the single binding /A/B.skel:skeleton =
/// </A/MyChar_skeleton> will be authored.
class PxrUsdMaya_SkelBindingsWriter {
public:
    PxrUsdMaya_SkelBindingsWriter();

    /// Informs the skel bindings writer that the \p boundPrim under the
    /// \p usdSkelRoot is skeletally bound to the skeleton rooted at the Maya
    /// joint given by \p mayaSkeletonDagPath.
    /// (When WriteSkelBindings is later invoked, a binding for
    /// \p mayaSkeletonDagPath will be authored at or above \p boundPrim, and
    /// at or below \p usdSkelRoot.)
    /// \p usdSkelRootWasAutoGenerated is a hint to WriteSkelBindings for
    /// generating more informative warnings, but otherwise does not affect
    /// the binding computation.
    void MarkBinding(const SdfPath& boundPrim, const SdfPath& usdSkelRoot,
            const MDagPath& mayaSkeletonDagPath,
            bool usdSkelRootWasAutoGenerated);

    /// Writes the final minimal set of skel bindings into the stage. See the
    /// class description for more information on how this works.
    bool WriteSkelBindings(const UsdStagePtr& stage, bool stripNamespaces) const;

private:
    struct SkelRootData {
        /// A path is in the table if it or any descendant has skinning.
        /// A path's value is its bound skeleton DAG path if there is binding
        /// at that specific prim, or the invalid DAG path otherwise.
        SdfPathTable<MDagPath> skinnedPaths;
        /// All of the Maya skeletons bound under this SkelRoot.
        std::set<MDagPath, PxrUsdMayaUtil::cmpDag> skeletonDagPaths;
        /// Whether this SkelRoot was auto-generated during SkelRoot
        /// auto-discovery for skinClusters. (Does not affect binding logic.)
        bool autoGenerated = false;
    };

    /// Mapping of SkelRoot prim paths to a SkelRootData struct that tracks
    /// the bindings underneath that SkelRoot.
    /// The skel bindings writer deals with each SkelRoot's subtree separately.
    /// Note that this works because _VerifyOrMakeSkelRoot in
    /// MayaMeshWriter_Skin enforces that there are no nested SkelRoots.
    std::unordered_map<SdfPath, SkelRootData, SdfPath::Hash> _skelRootMap;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_SKELBINDINGSWRITER_H
