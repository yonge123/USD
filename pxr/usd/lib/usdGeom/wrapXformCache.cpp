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
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include <boost/python/class.hpp>

PXR_NAMESPACE_OPEN_SCOPE


using namespace boost::python;

void wrapUsdGeomXformCache()
{
    typedef UsdGeomXformCache XformCache;

    class_<XformCache>("XformCache")
        .def(init<UsdTimeCode>(arg("time")))
        .def("GetLocalToWorldTransform",
             &XformCache::GetLocalToWorldTransform, arg("prim"))
        .def("GetParentToWorldTransform",
             &XformCache::GetParentToWorldTransform, arg("prim"))
        .def("Clear", &XformCache::Clear)
        .def("SetTime", &XformCache::SetTime, arg("time"))
        .def("GetTime", &XformCache::GetTime)

        .def("SetWorldPath", &XformCache::SetWorldPath, arg("path"))
        .def("GetWorldPath", &XformCache::GetWorldPath,
                return_value_policy<return_by_value>())

        .def("Swap", &XformCache::Swap, arg("other"))
        ;
}

PXR_NAMESPACE_CLOSE_SCOPE

