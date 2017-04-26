set(DIRS
    ar
    kind
    sdf
    pcp
    usd
    usdAi
    usdGeom
    usdShade
    usdHydra
    usdRi
    usdUI
    usdUtils
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

