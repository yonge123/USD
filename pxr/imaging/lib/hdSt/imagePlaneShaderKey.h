#ifndef HDST_IMAGEPLANE_SHADER_KEY_H
#define HDST_IMAGEPLANE_SHADER_KEY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/base/tf/token.h"
#include "geometricShader.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdSt_ImagePlaneShaderKey {
    HdSt_ImagePlaneShaderKey();
    ~HdSt_ImagePlaneShaderKey();

    const TfToken& GetGlslfxFile() const { return glslfx; }
    const TfToken* GetVS() const { return &VS[0]; }
    const TfToken* GetTCS() const { return nullptr; }
    const TfToken* GetTES() const { return nullptr; }
    const TfToken* GetGS() const { return nullptr; }
    const TfToken* GetFS() const { return &FS[0]; }
    bool IsCullingPass() const { return false; }
    HdCullStyle GetCullStyle() const { return HdCullStyleDontCare; }
    HdPolygonMode GetPolygonMode() const { return HdPolygonModeFill; }
    HdSt_GeometricShader::PrimitiveType GetPrimitiveType() const {
        return HdSt_GeometricShader::PrimitiveType::PRIM_MESH_COARSE_TRIANGLES;
    }

    bool IsFaceVarying() const { return false; }

    TfToken glslfx;
    TfToken VS[3];
    TfToken FS[4];
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif

