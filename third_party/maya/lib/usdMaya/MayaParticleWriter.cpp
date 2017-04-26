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

#include "usdMaya/MayaParticleWriter.h"

#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/gf/vec3f.h"

#include <maya/MFnParticleSystem.h>
#include <maya/MVectorArray.h>
#include <maya/MAnimControl.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    template <typename T>
    inline T _convertVector(const MVector& v) {
        // I wish we had concepts
        return T(static_cast<typename T::ScalarType>(v.x),
                 static_cast<typename T::ScalarType>(v.y),
                 static_cast<typename T::ScalarType>(v.z));
    }

    template <typename T>
    VtArray<T> _convertVectorArray(const MVectorArray& a) {
        VtArray<T> ret;
        const auto count = a.length();
        ret.resize(count);
        for (auto i = decltype(count){0}; i < count; ++i) {
            ret[i] = _convertVector<T>(a[i]);
        }
        return ret;
    }

    template <typename T>
    VtArray<T> _convertArray(const MDoubleArray& a) {
        VtArray<T> ret; // All hail to RVO
        const auto count = a.length();
        ret.resize(count);
        for (auto i = decltype(count){0}; i < count; ++i) {
            ret[i] = static_cast<T>(a[i]);
        }
        return ret;
    }
}

MayaParticleWriter::MayaParticleWriter(
    MDagPath& iDag, UsdStageRefPtr stage, const JobExportArgs& iArgs)
    : MayaTransformWriter(iDag, stage, iArgs) {
}

UsdPrim MayaParticleWriter::write(const UsdTimeCode &usdTime) {
    auto primSchema = UsdGeomPoints::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    auto prim = primSchema.GetPrim();
    TF_AXIOM(prim);

    writeTransformAttrs(usdTime, primSchema);
    writeParams(usdTime, primSchema);

    return prim;
}

void MayaParticleWriter::writeParams(const UsdTimeCode& usdTime, UsdGeomPoints& points) {
    if (usdTime.IsDefault() == isShapeAnimated()) {
        return;
    }

    MStatus status;
    MFnParticleSystem particleFn;
    particleFn.setObject(getDagPath().node());

    auto currTime = MAnimControl::currentTime();
    const auto playFromCachePlug = particleFn.findPlug("playFromCache");
    if (!playFromCachePlug.isNull() && playFromCachePlug.asBool()) {
        particleFn.evaluateDynamics(currTime, false);
    }

    MVectorArray positions;
    MVectorArray velocities;
    MDoubleArray ids;
    particleFn.getPerParticleAttribute("particleId", ids);
    particleFn.velocity(velocities);
    particleFn.position(positions);

    points.GetPointsAttr().Set(_convertVectorArray<GfVec3f>(positions), usdTime);
    points.GetVelocitiesAttr().Set(_convertVectorArray<GfVec3f>(velocities), usdTime);
    points.GetIdsAttr().Set(_convertArray<long>(ids), usdTime);
}

PXR_NAMESPACE_CLOSE_SCOPE
