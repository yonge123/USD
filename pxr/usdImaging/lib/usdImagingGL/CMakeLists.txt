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
    set(GLSLANG_INCLUDES ${GLSLANG_REPOSITORY}/SPIRV ${GLSLANG_REPOSITORY}/glslang/Include ${GLSLANG_REPOSITORY}/glslang/Public)
    # we also start looking for glslang from now on
    find_library(
            GLSLANG_LIBRARY
            NAMES libglslang.a
            PATHS ${GLSLANG_LIBRARIES_PATH}
    )

    find_library(
            OGLCOMPILER_LIBRARY
            NAMES libOGLCompiler.a
            PATHS ${GLSLANG_LIBRARIES_PATH}
    )

    find_library(
            OSDEPENDENT_LIBRARY
            NAMES libOSDependent.a
            PATHS ${GLSLANG_LIBRARIES_PATH}
    )

    find_library(
            HLSL_LIBRARY
            NAMES libHLSL.a
            PATHS ${GLSLANG_LIBRARIES_PATH}
    )

    find_library(
            SPIRV_LIBRARY
            NAMES libSPIRV.a
            PATHS ${GLSLANG_LIBRARIES_PATH}
    )

    find_library(
            SPIRVREMAPPER_LIBRARY
            NAMES libSPVRemapper.a
            PATHS ${GLSLANG_LIBRARIES_PATH}
    )

    set(GLSLANG_LIBRARIES ${GLSLANG_LIBRARY} ${OGLCOMPILER_LIBRARY} ${OSDEPENDENT_LIBRARY}
            ${HLSL_LIBRARY} ${SPIRV_LIBRARY} ${SPIRVREMAPPER_LIBRARY})
    file(GLOB HK_SOURCES hk/*.cpp)
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
        ${OIIO_LIBRARIES}
        ${VULKAN_LIBRARIES}
        ${GLSLANG_LIBRARIES}

    INCLUDE_DIRS
        ${PYTHON_INCLUDE_PATH}
        ${PYTHON_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
        ${GLSLANG_INCLUDES}

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
        ${HK_SOURCES}

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
