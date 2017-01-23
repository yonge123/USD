set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdAi)

pxr_shared_library(usdAi
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdShade

    PUBLIC_CLASSES
        aiShader

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAiShader.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
)

