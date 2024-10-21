# ======================================================================================================================
# EXTERNAL DEPENDENCIES
# ======================================================================================================================
include(FetchContent)

# For Windowing context
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4)
FetchContent_MakeAvailable(glfw)

# For basic linear algebra
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1)
FetchContent_MakeAvailable(glm)

# For debug logging
FetchContent_Declare(
        plog
        GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
        GIT_TAG 1.1.10)
FetchContent_MakeAvailable(plog)

# ======================================================================================================================
# LIBARY SOURCE FILES
# ======================================================================================================================
set(TORPEDO_GRAPHICS_SOURCES
        src/Camera.cpp
        src/Composable.cpp
        src/Drawable.cpp
        src/Geometry.cpp
        src/Light.cpp
        src/Material.cpp
        src/MaterialInstance.cpp
        src/Renderer.cpp
        src/Scene.cpp
        src/View.cpp
        src/camera/PerspectiveCamera.cpp
        src/light/DistantLight.cpp
        src/light/PointLight.cpp
        src/material/PhongMaterial.cpp
        src/renderer/ForwardRenderer.cpp
        src/renderer/StandardRenderer.cpp
)

# ======================================================================================================================
# TORPEDO GRAPHICS LIBARY
# ======================================================================================================================
set(TARGET torpedo_graphics)
add_library(${TARGET} STATIC)
add_library(torpedo::graphics ALIAS ${TARGET})
set_target_properties(${TARGET} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF COMPILE_WARNING_AS_ERROR ON)
target_sources(${TARGET} PRIVATE ${TORPEDO_GRAPHICS_SOURCES})
target_compile_definitions(${TARGET} PUBLIC GLFW_INCLUDE_NONE GLM_FORCE_DEPTH_ZERO_TO_ONE)

# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)

# Dependencies
target_link_libraries(${TARGET} PUBLIC torpedo::bootstrap)
target_link_libraries(${TARGET} PUBLIC torpedo::foundation)
target_link_libraries(${TARGET} PUBLIC glfw)
target_link_libraries(${TARGET} PUBLIC glm::glm)
target_link_libraries(${TARGET} PUBLIC plog::plog)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (WIN32)
        # The current Vulkan-Hpp implementation internally uses strncpy which is marked deprecated for Clang on Windows.
        # This would trigger a warning which is treated as compilation error, so we disable it here
        target_compile_options(${TARGET} PRIVATE -Wno-deprecated-declarations)
    else ()
        # Use libc++ for Clang on non-Windows platforms
        target_compile_options(${TARGET} PRIVATE -stdlib=libc++)
        target_link_libraries(${TARGET} PRIVATE :libc++.a :libc++abi.a)
    endif ()
endif()