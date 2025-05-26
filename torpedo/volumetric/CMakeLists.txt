# MODULE SOURCE FILES
# -------------------
set(TORPEDO_VOLUMETRIC_SOURCES
        src/GaussianEngine.cpp
        src/GaussianGeometry.cpp
        src/miniply.cpp)


# TORPEDO VOLUMETRIC MODULE
# ------------------------
set(TARGET torpedo_volumetric)
torpedo_define_module(${TARGET} "${TORPEDO_VOLUMETRIC_SOURCES}")
# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
target_include_directories(${TARGET} PRIVATE src)
# Dependencies
target_link_libraries(${TARGET} PUBLIC torpedo::rendering)


# SHADER ASSETS
# -------------
# Where to look for assets dir, access (and also return here) via TORPEDO_VOLUMETRIC_ASSETS_DIR
torpedo_define_assets_dir(${TARGET} VOLUMETRIC ${CMAKE_CURRENT_BINARY_DIR} TORPEDO_VOLUMETRIC_ASSETS_DIR)
# Shader assets
set(TORPEDO_VOLUMETRIC_SHADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/gaussian/project.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/gaussian/prefix.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/gaussian/keygen.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/gaussian/radix-upfront.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/gaussian/radix-prefix.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/gaussian/radix-binning.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/gaussian/range.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/gaussian/blend.slang)
torpedo_compile_slang(${TARGET} "${TORPEDO_VOLUMETRIC_ASSETS_DIR}/gaussian" "${TORPEDO_VOLUMETRIC_SHADERS}")
target_link_libraries(${TARGET} PRIVATE "${TARGET}_spirv_binaries")
