# Main executable
# ---------------
set(TARGET HelloTriangle)
add_executable(${TARGET} main.cpp)

# Dependencies
# ------------
target_link_libraries(${TARGET} PRIVATE torpedo::rendering)
target_link_libraries(${TARGET} PRIVATE torpedo::utility)

# Where to look for compiled shader files
# ---------------------------------------
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-configuration generators (Visual Studio, Xcode)
    set(HELLO_TRIANGLE_SHADER_SPV_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/shaders")
else()
    # Single-configuration generators (Makefiles, Ninja)
    set(HELLO_TRIANGLE_SHADER_SPV_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
endif()

# Compile shaders as part of build, using torpedo's shader compilation function
# -----------------------------------------------------------------------------
set(HELLO_TRIANGLE_SHADER_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/simple.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/simple.frag
)
torpedo_compile_glsl(${TARGET} ${HELLO_TRIANGLE_SHADER_SPV_DIR} "${HELLO_TRIANGLE_SHADER_FILES}")
# The function registers an interface target named ${TARGET}_spirv_binaries. When linking against this target,
# recompilation will be triggered everytime shader sources are changed
target_link_libraries(${TARGET} PRIVATE "${TARGET}_spirv_binaries")

# Ony applicable if compiling against Clang
# -----------------------------------------
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