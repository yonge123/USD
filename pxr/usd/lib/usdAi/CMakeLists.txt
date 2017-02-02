set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdAi)

pxr_shared_library(usdAi
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdGeom
        usdShade

    PUBLIC_CLASSES
        tokens
        aiMaterialAPI
        aiNodeAPI
        aiProcedural
        aiShader
        aiVolume

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAiMaterialAPI.cpp
        wrapAiNodeAPI.cpp
        wrapAiProcedural.cpp
        wrapAiShader.cpp
        wrapAiVolume.cpp
        wrapTokens.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
)
