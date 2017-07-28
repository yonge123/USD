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

#ifndef PXRUSDMAYA_MAYAINSTANCER_WRITER_H
#define PXRUSDMAYA_MAYAINSTANCER_WRITER_H

#include "pxr/pxr.h"
#include "usdMaya/MayaTransformWriter.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomPointInstancer;

class MayaInstancerWriter : public MayaTransformWriter
{
public:
    MayaInstancerWriter(const MDagPath& iDag,
                        const SdfPath& uPath,
                        bool instanceSource,
                        usdWriteJobCtx& jobCtx);
    virtual ~MayaInstancerWriter() {}

    virtual void write(const UsdTimeCode& usdTime) override;

    // TODO: Check this properly, static particles are uncommon, but used.
    virtual bool isShapeAnimated() const override { return true; }
private:
    void writeParams(const UsdTimeCode& usdTime, UsdGeomPointInstancer& instancer);

    int getPathIndex(const MDagPath& path, UsdGeomPointInstancer& instancer);
    // Also, note, we should replace this with a sorted vector.
    // This is required to filter paths that doesn't have a valid usd prim.
    PxrUsdMayaUtil::MDagPathMap<int>::Type mRelationships;
};

typedef std::shared_ptr<MayaInstancerWriter> MayaInstancerWriterPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_MAYAINSTANCER_WRITER_H
