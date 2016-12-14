set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImagingGL)

# TODO: do this properly in the future
find_path(VULKAN_HEADER
          NAMES vulkan/vulkan.h)
find_path(VULKAN_HPP_HEADER
          NAMES vulkan/vulkan.hpp)
if (VULKAN_HEADER AND VULKAN_HPP_HEADER)
    # I couldn't find a flag to append definitions to the compiled code
    add_compile_options("-DENABLE_HK_ENGINE")
    set(VULKAN_LIBRARIES vulkan)
    set(HK_CLASSES hkEngine)
else ()
    set(VULKAN_LIBRARIES "")
    set(HK_CLASSES "")
endif ()

# Note: both python include directives are included for compatibility.
pxr_shared_library(usdImagingGL
    LIBRARIES
        gf
        tf
        plug
        tracelite
        vt
        work
        garch
        glf
        hd
        hdx
        pxOsd
        sdf
        usd
        usdGeom
        usdShade
        usdHydra
        usdImaging
        ar
        ${Boost_PYTHON_LIBRARY}
        ${PYTHON_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${OPENGL_glu_LIBRARY}
        ${GLEW_LIBRARY}
        ${TBB_tbb_LIBRARY}
        ${TBB_tbbmalloc_LIBRARY}
        ${VULKAN_LIBRARIES}

    INCLUDE_DIRS
        ${PYTHON_INCLUDE_PATH}
        ${PYTHON_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        engine
        gl
        hdEngine
        refEngine
        taskDelegate

    PRIVATE_CLASSES
        defaultTaskDelegate
        ${HK_CLASSES}

    PRIVATE_HEADERS
        unitTestGLDrawing.h

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapGL.cpp

    PYTHON_FILES
        __init__.py
)

if (X11_FOUND OR APPLE)
    pxr_build_test(testUsdImagingGLBasicDrawing
        LIBRARIES
            hd
            garch
            glf
            usd
            usdUtils
            usdImaging
            usdImagingGL
            sdf
        CPPFILES
            unitTestGLDrawing.cpp
            testenv/testUsdImagingGLBasicDrawing.cpp
    )
endif()
