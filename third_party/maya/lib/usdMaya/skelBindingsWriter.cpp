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
#include "usdMaya/skelBindingsWriter.h"

#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdUtils/authoring.h"

#include "usdMaya/MayaSkeletonWriter.h"
#include "usdMaya/translatorUtil.h"

PXR_NAMESPACE_OPEN_SCOPE

PxrUsdMaya_SkelBindingsWriter::PxrUsdMaya_SkelBindingsWriter()
{
}

void
PxrUsdMaya_SkelBindingsWriter::MarkBinding(
    const SdfPath& boundPrim,
    const SdfPath& usdSkelRoot,
    const MDagPath& mayaSkeletonDagPath,
    bool usdSkelRootWasAutoGenerated)
{
    SkelRootData& data = _skelRootMap[usdSkelRoot];
    data.skinnedPaths[boundPrim] = mayaSkeletonDagPath;
    data.skeletonDagPaths.insert(mayaSkeletonDagPath);
    data.autoGenerated |= usdSkelRootWasAutoGenerated;
}

/// Checks that the given relationship has a single specified target.
/// If not, replaces the targets and issues a coding error.
static void
_CheckRelHasOneTarget(const UsdRelationship& rel, const SdfPath& target)
{
    SdfPathVector existingTargets;
    rel.GetTargets(&existingTargets);
    if (existingTargets.size() == 1 && existingTargets[0] == target) {
        return;
    }

    TF_CODING_ERROR("Skeleton rels binding site <%s> is already bound to "
            "different target(s). Expected all skinned meshes at or below the "
            "binding site to have target <%s>.",
            rel.GetPath().GetText(),
            target.GetText());
    rel.ClearTargets(false);
    rel.AddTarget(target);
}

static void
_WriteSkelBindings(
    const SdfPath& bindingSite,
    const UsdStagePtr& stage,
    const MDagPath& skeletonDagPath,
    bool stripNamespaces)
{
    if (!skeletonDagPath.isValid()) {
        TF_CODING_ERROR("Skeleton '%s' is not valid",
                skeletonDagPath.fullPathName().asChar());
        return;
    }

    const UsdPrim& bindingPrim = stage->GetPrimAtPath(bindingSite);
    const UsdSkelBindingAPI bindingAPI = PxrUsdMayaTranslatorUtil
            ::GetAPISchemaForAuthoring<UsdSkelBindingAPI>(bindingPrim);

    SdfPath skeletonPath = MayaSkeletonWriter::GetSkeletonPath(skeletonDagPath, stripNamespaces);
    if (stage->GetPrimAtPath(skeletonPath)) {
        if (UsdRelationship existingRel = bindingAPI.GetSkeletonRel()) {
            _CheckRelHasOneTarget(existingRel, skeletonPath);
        }
        else {
            bindingAPI.CreateSkeletonRel().AddTarget(skeletonPath);
        }
    }
    SdfPath animPath = MayaSkeletonWriter::GetAnimationPath(skeletonDagPath, stripNamespaces);
    if (stage->GetPrimAtPath(animPath)) {
        if (UsdRelationship existingRel = bindingAPI.GetAnimationSourceRel()) {
            _CheckRelHasOneTarget(existingRel, animPath);
        }
        else {
            bindingAPI.CreateAnimationSourceRel().AddTarget(animPath);
        }
    }
}

/// Traverses the table of all skinned paths to find the paths that are bound to
/// skeletonDagPath, but doesn't include skinned descendants (prune traversal
/// once we hit a skinned path).
/// e.g. assume:
///     - /A unbound
///     - /A/B bound to |Char1_skeleton
///     - /A/B/C bound to |Char1_skeleton
///     - /A/X bound to |Char2_skeleton
///     - /A/Y bound to |Char1_skeleton
/// then _GetRootBoundPaths(table, |Char1_skeleton) = set(</A/B>, </A/Y>).
SdfPathSet
_GetRootBoundPaths(
    const SdfPathTable<MDagPath>& table,
    const MDagPath& skeletonDagPath)
{
    SdfPathSet includedRootPaths;
    auto iter = table.begin();
    while (iter != table.end()) {
        const SdfPath& currentPrimPath = iter->first;
        const MDagPath& currentSkeletonDagPath = iter->second;
        if (currentSkeletonDagPath == skeletonDagPath) {
            // GetNextSubtree() is like UsdPrimRange::PruneChildren().
            // Since this path is included in the set, we can skip its subtree.
            includedRootPaths.insert(currentPrimPath);
            iter = iter.GetNextSubtree();
        }
        else {
            // Only advance the iterator if we didn't call GetNextSubtree().
            ++iter;
        }
    }

    return includedRootPaths;
}

bool
PxrUsdMaya_SkelBindingsWriter::WriteSkelBindings(
    const UsdStagePtr& stage,
    bool stripNamespaces) const
{
    for (const auto& skelRootPathAndData : _skelRootMap) {
        const SdfPath& skelRootPath = skelRootPathAndData.first;
        const SkelRootData& skelRootData = skelRootPathAndData.second;

        const UsdPrim skelRootPrim = stage->GetPrimAtPath(skelRootPath);
        if (!skelRootPrim) {
            TF_CODING_ERROR("SkelRoot <%s> is missing", skelRootPath.GetText());
            continue;
        }

        // If this SkelRoot has only one skeleton bound, then author that
        // single skeleton's bindings directly at the SkelRoot.
        if (skelRootData.skeletonDagPaths.size() == 1) {
            _WriteSkelBindings(
                    skelRootPath,
                    stage,
                    *skelRootData.skeletonDagPaths.begin(),
                    stripNamespaces);
            continue;
        }

        // This SkelRoot has multiple skeletons bound under it. If the
        // SkelRoot was previously auto-generated during this export, emit a
        // warning.
        if (skelRootData.autoGenerated) {
            std::vector<std::string> skeletonDagPathNames;
            for (const MDagPath& dagPath : skelRootData.skeletonDagPaths) {
                skeletonDagPathNames.push_back(dagPath.fullPathName().asChar());
            }
            TF_WARN("The auto-generated SkelRoot <%s> has multiple skeletons "
                    "bound in its hierarchy: %s",
                    skelRootPath.GetText(),
                    TfStringJoin(skeletonDagPathNames).c_str());
        }

        // Invert the list of skinnedPaths to obtain the paths-to-ignore set.
        // This is a set of paths under the SkelRoot which do not have skinning
        // authored at the prim or any of its descendants; they can be ignored
        // in UsdUtilsComputeCollectionIncludesAndExcludes because it doesn't
        // matter whether they are included under the skel binding or not.
        // This takes advantage of the fact that inserting /A/B/C into an
        // SdfPathTable inserts /A/B and /A as well.
        UsdUtilsPathHashSet pathsToIgnore;
        for (const UsdPrim& p : skelRootPrim.GetDescendants()) {
            if (!skelRootData.skinnedPaths.count(p.GetPath())) {
                pathsToIgnore.insert(p.GetPath());
            }
        }

        // For each skeleton DAG path bound under this SkelRoot, compute the
        // location to author its binding. This is analogous to figuring out
        // where to author a collection "includes" relationship.
        // Consider that authoring a skel:skeleton binding is like authoring a
        // collection that includes that prim's subtree. Thus, we can use
        // UsdUtilsComputeCollectionIncludesAndExcludes to figure out a set of
        // include paths, and use the include paths to author the skel:skeleton
        // bindings.
        for (const MDagPath& skeletonDagPath : skelRootData.skeletonDagPaths) {
            SdfPathSet includedRootPaths = _GetRootBoundPaths(
                    skelRootData.skinnedPaths, skeletonDagPath);
            SdfPathVector pathsToInclude;
            SdfPathVector pathsToExclude;
            UsdUtilsComputeCollectionIncludesAndExcludes(
                    includedRootPaths, stage,
                    &pathsToInclude, &pathsToExclude,
                    /*minInclusionRatio*/ 1.0,              // no exclude paths
                    /*maxNumExcludesBelowInclude*/ 1u,      // doesn't matter
                    /*minIncludeExcludeCollectionSize*/ 1u, // always compute
                    pathsToIgnore);
            for (const SdfPath& path : pathsToInclude) {
                _WriteSkelBindings(path, stage, skeletonDagPath, stripNamespaces);
            }
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
