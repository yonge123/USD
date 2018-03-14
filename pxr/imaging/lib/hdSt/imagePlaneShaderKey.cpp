#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/imagePlaneShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,      "imagePlane.glslfx"))
    ((mainVS,          "ImagePlane.Vertex"))
    ((commonFS,        "Fragment.CommonTerminals"))
    ((surfaceFS,       "Fragment.Surface"))
    ((mainFS,          "ImagePlane.Fragment"))
);

HdSt_ImagePlaneShaderKey::HdSt_ImagePlaneShaderKey()
    : glslfx(_tokens->baseGLSLFX) {
    VS[0] = _tokens->mainVS;
    VS[1] = TfToken();

    FS[0] = _tokens->surfaceFS;
    FS[1] = _tokens->commonFS;
    FS[2] = _tokens->mainFS;
    FS[3] = TfToken();
}

HdSt_ImagePlaneShaderKey::~HdSt_ImagePlaneShaderKey() {

}

PXR_NAMESPACE_CLOSE_SCOPE
