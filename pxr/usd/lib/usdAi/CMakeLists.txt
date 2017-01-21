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
        aiOpenVDBVolumeShader
        aiShader
        aiShape
        aiVolumeOpenVDB
        tokens

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
)

