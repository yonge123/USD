//
// Copyright 2017 Pixar
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

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/base/gf/math.h"

#include <vector>
#include <array>

PXR_NAMESPACE_USING_DIRECTIVE

// No initializer list constructor! This is for making things more readable.
VtVec3fArray _convertArray(const std::vector<GfVec3f>& v)
{
    VtVec3fArray ret;
    ret.reserve(v.size());
    for (const auto& e: v) {
        ret.push_back(e);
    }
    return ret;
}

bool _verifyArrays(const VtVec3fArray& v1, const VtVec3fArray& v2, double epsilon = 1e-4)
{
    const auto count = v1.size();
    if (count != v2.size()) { return false; }
    for (auto i = decltype(count){0}; i < count; ++i) {
        const auto e1 = v1[i];
        const auto e2 = v2[i];
        if (!GfIsClose(e1[0], e2[0], epsilon) ||
            !GfIsClose(e1[1], e2[1], epsilon) ||
            !GfIsClose(e1[2], e2[2], epsilon)) {
            return false;
        }
    }
    return true;
}

VtVec3fArray _addArrays(const VtVec3fArray& v1, const VtVec3fArray& v2, double value) {
    const auto fvalue = static_cast<float>(value);
    VtVec3fArray ret;
    const auto count = v1.size();
    ret.resize(count);
    for (auto i = decltype(count){0}; i < count; ++i) {
        ret[i] = v1[i] + v2[i] * fvalue;
    }
    return ret;
}

constexpr auto framesPerSecond = 24.0;

const SdfPath pointsPath("/points");

constexpr double d10 = 1.0;
constexpr double d20 = 2.0;
constexpr double d30 = 3.0;
constexpr double d02 = 0.2;
constexpr double d04 = 0.4;
constexpr double d08 = 0.8;

UsdTimeCode frame1(d10);
UsdTimeCode frame12(d10 + d02);
UsdTimeCode frame14(d10 + d04);
UsdTimeCode frame2(d20);
UsdTimeCode frame22(d20 + d02);
UsdTimeCode frame28(d20 + d08);
UsdTimeCode frame3(d30);

VtVec3fArray positions1 = _convertArray({GfVec3f(1.0f, 2.0f, 3.0f)});
VtVec3fArray positions2 = _convertArray({GfVec3f(1.0f, 2.0f, 3.0f), GfVec3f(6.0f, 3.0f, 2.0f)});
VtVec3fArray positions3 = _convertArray({GfVec3f(8.0f, 3.0f, 5.0f),
                                         GfVec3f(-4.0f, -5.0f, 17.0f), GfVec3f(-4.0f, 23.0f, 12.0f)});

VtVec3fArray velocities1 = _convertArray({GfVec3f(10.0f, 10.0f, 10.0f)});
VtVec3fArray velocities2 = _convertArray({GfVec3f(4.0f, 12.0f, 75.0f), GfVec3f(-4.0f, -83.0f, 65.0f)});
VtVec3fArray velocities3 = _convertArray({GfVec3f(-8.0f, 13.0f, -5.0f),
                                          GfVec3f(-24.0f, -45.0f, 17.0f), GfVec3f(-44.0f, 23.0f, 112.0f)});

void TestUsdGeomPointsComputePositions()
{
    auto stage = UsdStage::CreateInMemory("test.usda");
    stage->SetFramesPerSecond(framesPerSecond);

    auto points = UsdGeomPoints::Define(stage, pointsPath);

    TF_VERIFY(points,
              "Failed to create prim at %s",
              pointsPath.GetText());
}

int main()
{
    TestUsdGeomPointsComputePositions();
}
