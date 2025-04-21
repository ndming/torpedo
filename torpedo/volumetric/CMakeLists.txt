# MODULE SOURCE FILES
# -------------------
set(TORPEDO_VOLUMETRIC_SOURCES
        src/GaussianEngine.cpp)


# TORPEDO VOLUMETRIC MODULE
# ------------------------
set(TARGET torpedo_volumetric)
torpedo_define_module(${TARGET} "${TORPEDO_VOLUMETRIC_SOURCES}" PUBLIC)
# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
# Dependencies
target_link_libraries(${TARGET} PUBLIC torpedo::rendering)
target_link_libraries(${TARGET} PUBLIC vsg::vsg)


# SHADER ASSETS
# -------------
# Where to look for assets dir, access (and also return here) via TORPEDO_VOLUMETRIC_ASSETS_DIR
torpedo_define_assets_dir(${TARGET} VOLUMETRIC ${CMAKE_CURRENT_BINARY_DIR} TORPEDO_VOLUMETRIC_ASSETS_DIR)
# Shader assets
set(TORPEDO_VOLUMETRIC_SHADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/slang/prepare.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/slang/prefix.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/slang/keygen.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/slang/radix.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/slang/range.slang
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/slang/forward.slang)
torpedo_compile_slang(${TARGET} "${TORPEDO_VOLUMETRIC_ASSETS_DIR}/slang" "${TORPEDO_VOLUMETRIC_SHADERS}")
target_link_libraries(${TARGET} PRIVATE "${TARGET}_spirv_binaries")
