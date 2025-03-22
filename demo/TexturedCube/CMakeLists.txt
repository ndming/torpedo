# Main executable
# ---------------
set(TARGET TexturedCube)
add_executable(${TARGET} main.cpp ext/stb.cpp)
target_include_directories(${TARGET} PRIVATE ext)

# Dependencies
# ------------
target_link_libraries(${TARGET} PRIVATE torpedo::rendering)

# Where to look for texture images
# ---------------------------------------
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-configuration generators (Visual Studio, Xcode)
    set(TEXTURED_CUBE_TEXTURE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/textures")
else()
    # Single-configuration generators (Makefiles, Ninja)
    set(TEXTURED_CUBE_TEXTURE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/textures")
endif()

# Copy texture assets to build directory
# --------------------------------------
set(TEXTURED_CUBE_TEXTURE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/textures/diffuse.png
        ${CMAKE_CURRENT_SOURCE_DIR}/textures/specular.png
)
tordemo_copy_resource(${TARGET} ${TEXTURED_CUBE_TEXTURE_OUTPUT_DIR} "${TEXTURED_CUBE_TEXTURE_FILES}")

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