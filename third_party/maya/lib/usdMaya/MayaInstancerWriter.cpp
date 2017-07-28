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
#include "usdMaya/MayaInstancerWriter.h"

#include "pxr/usd/usdGeom/pointInstancer.h"

#include <maya/MFnInstancer.h>
#include <maya/MDagPathArray.h>
#include <maya/MDagPathArray.h>
#include <maya/MMatrixArray.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MQuaternion.h>
#include <maya/MFnParticleSystem.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MTime.h>

PXR_NAMESPACE_OPEN_SCOPE

MayaInstancerWriter::MayaInstancerWriter(const MDagPath& iDag,
                    const SdfPath& uPath,
                    bool instanceSource,
                    usdWriteJobCtx& jobCtx)
    : MayaTransformWriter(iDag, uPath, instanceSource, jobCtx) {
    if (jobCtx.getArgs().exportInstances) {
        auto primSchema = UsdGeomPointInstancer::Define(getUsdStage(), getUsdPath());
        TF_AXIOM(primSchema);
        mUsdPrim = primSchema.GetPrim();
        TF_AXIOM(mUsdPrim);
    } else {
        setValid(false);
    }
}

void MayaInstancerWriter::write(const UsdTimeCode& usdTime) {
    UsdGeomPointInstancer primSchema(mUsdPrim);

    // We need to manually write out transform attrs, because there is no separate
    // shape and transform, the instancer is a transform node.
    writeTransformAttrs(usdTime, primSchema);


    // Maya flattens out instancing groups for us (even though USD supports it!),
    // so the initial implementation will be to add each instance as a separate particle.
    // Later on we can create groups for each combination of paths, and add them below the pointInstancer.

    // We are using the write job's behavior to relate to other nodes.
    // The write job first writes out everything with the default time,
    // then calls the write for all time values. So at the first non default time
    // we'll have all the instances already exported and ready to be used.
    // We can't use the jobCtx's MDagPath to USD path function, because
    // that doesn't check if the dagpaths will be exported + we don't want to care
    // about checking how instancing export + merge shapes and transforms affects the source node's path.
    writeParams(usdTime, primSchema);
}

void MayaInstancerWriter::writeParams(const UsdTimeCode& usdTime, UsdGeomPointInstancer& instancer) {
    if (usdTime.IsDefault() == isShapeAnimated()) { return; }

    MStatus status;
    MFnInstancer mayaInstancer(getDagPath(), &status);
    if (!status || mayaInstancer.particleCount() == 0) { return; }

    MDagPathArray paths;
    MMatrixArray matrices;
    MIntArray particleStartIndices;
    MIntArray mayaIndices;

    status = mayaInstancer.allInstances(paths, matrices, particleStartIndices, mayaIndices);

    if (!status ||
        paths.length() == 0 ||
        matrices.length() == 0 ||
        particleStartIndices.length() < 2 ||
        mayaIndices.length() == 0) { return; }

    // Accessing extra data from the driver.
    auto hasVelocity = false;
    MVectorArray mayaVelocities;
    const auto instancerObject = getDagPath().node();
    MFnDependencyNode mayaInstancerNode(instancerObject, &status);
    if (status) {
        auto inputPointsPlug = mayaInstancerNode.findPlug("inputPoints");
        MPlugArray conns;
        inputPointsPlug.connectedTo(conns, true, false);
        if (conns.length() > 0) {
            const auto driverObject = conns[0].node();
            if (driverObject.hasFn(MFn::kParticle) || driverObject.hasFn(MFn::kNParticle)) {
                MFnParticleSystem PS(driverObject);
                PS.velocity(mayaVelocities);
                // TODO: check if the velocities are zero!
                hasVelocity = mayaVelocities.length() == matrices.length();
            } else {
                MFnDependencyNode driverNode(driverObject);
                // This is a known bug with Mash.
                if (driverNode.typeName() != "MASH_Waiter"){
                    MFnArrayAttrsData instancerData(inputPointsPlug.asMObject(), &status);
                    if (status) {
                        auto vectorType = MFnArrayAttrsData::kVectorArray;
                        if (instancerData.checkArrayExist("velocity", vectorType)) {
                            mayaVelocities = instancerData.getVectorData("velocity");
                            hasVelocity = mayaVelocities.length() == matrices.length();
                        }
                    }
                }
            }
        }
    }

    const auto reserveCount = mayaIndices.length();
    VtArray<int> indices; indices.reserve(reserveCount);
    VtArray<GfVec3f> translations; translations.reserve(reserveCount);
    VtArray<GfQuath> orientations; orientations.reserve(reserveCount);
    VtArray<GfVec3f> scales; scales.reserve(reserveCount);
    VtArray<GfVec3f> velocities; if (hasVelocity) { velocities.reserve(reserveCount); }
    const auto particleGroups = particleStartIndices.length() - 1;
    for (auto gid = decltype(particleGroups){0}; gid < particleGroups; ++gid) {
        MTransformationMatrix transformMatrix(matrices[gid]);
        const auto translation = transformMatrix.getTranslation(MSpace::kTransform);
        const auto orientation = transformMatrix.rotationOrientation();
        MVector scale(0.0, 0.0, 0.0);
        transformMatrix.getScale(&scale.x, MSpace::kTransform);
        const auto end = particleStartIndices[gid + 1];
        for (auto pid = particleStartIndices[gid]; pid < end; ++pid) {
            const auto id = mayaIndices[pid];
            const auto relationshipId = getPathIndex(paths[id], instancer);
            if (relationshipId != -1) {
                indices.push_back(relationshipId);
                translations.push_back(GfVec3f(
                    static_cast<float>(translation.x),
                    static_cast<float>(translation.y),
                    static_cast<float>(translation.z)));
                orientations.push_back(GfQuath(
                    static_cast<float>(orientation.w),
                    static_cast<float>(orientation.x),
                    static_cast<float>(orientation.y),
                    static_cast<float>(orientation.z)));
                scales.push_back(GfVec3f(
                    static_cast<float>(scale.x),
                    static_cast<float>(scale.y),
                    static_cast<float>(scale.z)));
                if (hasVelocity) {
                    const auto velocity = mayaVelocities[gid];
                    velocities.push_back(GfVec3f(
                        static_cast<float>(velocity.x),
                        static_cast<float>(velocity.y),
                        static_cast<float>(velocity.z)));
                }
            }
        }
    }

    instancer.GetProtoIndicesAttr().Set(indices, usdTime);
    instancer.GetPositionsAttr().Set(translations, usdTime);
    instancer.GetOrientationsAttr().Set(orientations, usdTime);
    instancer.GetScalesAttr().Set(scales, usdTime);
    if (hasVelocity) {
        instancer.GetVelocitiesAttr().Set(velocities, usdTime);
    }
}

int MayaInstancerWriter::getPathIndex(const MDagPath& path, UsdGeomPointInstancer& instancer) {
    auto handlePath = [&] (const SdfPath& usdPath) -> int {
        const int id = static_cast<int>(mRelationships.size());
        mRelationships.emplace(path, id);
        instancer.GetPrototypesRel().AppendTarget(usdPath);
        return id;
    };
    const auto it = mRelationships.find(path);
    if (it == mRelationships.end()) {
        const auto usdPath = mWriteJobCtx.getUsdPath(path);
        if (usdPath.IsEmpty()) {
            // If usdPath is empty, that means the object is not exported
            // so we are trying to trigger an export via querying the getMasterPath.
            const auto masterPath = mWriteJobCtx.getMasterPath(path);
            if (masterPath.IsEmpty()) {
                mRelationships.emplace(path, -1);
                return -1;
            } else {
                return handlePath(masterPath);
            }
        } else {
            return handlePath(usdPath);
        }
    } else {
        return it->second;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
