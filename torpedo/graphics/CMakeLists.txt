# ======================================================================================================================
# EXTERNAL DEPENDENCIES
# ======================================================================================================================
include(FetchContent)

# GLFW for Windowing context
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4)
FetchContent_MakeAvailable(glfw)

# plog for debug logging
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
        src/Engine.cpp
        src/Geometry.cpp
        src/Material.cpp
        src/MaterialInstance.cpp
        src/Mesh.cpp
        src/Renderer.cpp
        src/renderer/raster/ForwardRenderer.cpp
        src/renderer/raster/RasterRenderer.cpp
)

# ======================================================================================================================
# TORPEDO GRAPHICS LIBARY
# ======================================================================================================================
set(TARGET torpedo_graphics)
add_library(${TARGET} STATIC)
add_library(torpedo::graphics ALIAS ${TARGET})
set_target_properties(${TARGET} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF COMPILE_WARNING_AS_ERROR ON)
target_sources(${TARGET} PRIVATE ${TORPEDO_GRAPHICS_SOURCES})
target_compile_definitions(${TARGET} PUBLIC GLFW_INCLUDE_NONE)

# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
target_include_directories(${TARGET} PRIVATE src)

# Dependencies
target_link_libraries(${TARGET} PUBLIC torpedo::bootstrap)
target_link_libraries(${TARGET} PUBLIC torpedo::foundation)
target_link_libraries(${TARGET} PUBLIC glfw)
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