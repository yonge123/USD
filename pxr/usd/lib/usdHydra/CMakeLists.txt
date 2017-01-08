set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdHydra)

pxr_shared_library(usdHydra

    LIBRARIES
        tf
        usd
        usdShade
        usdGeom

    PUBLIC_CLASSES
        lookAPI
        primvar
        shader
        texture
        uvTexture
        ptexTexture
        udimTexture
        physicalLight
        distantLight
        sphereLight
        spotLight
        quadLight
        skyLight
        tokens

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapLookAPI.cpp
        wrapPrimvar.cpp
        wrapShader.cpp
        wrapTexture.cpp
        wrapUvTexture.cpp
        wrapPtexTexture.cpp
        wrapTokens.cpp
        wrapUdimTexture.cpp
        wrapPhysicalLight.cpp
        wrapDistantLight.cpp
        wrapSphereLight.cpp
        wrapSpotLight.cpp
        wrapQuadLight.cpp
        wrapSkyLight.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)

