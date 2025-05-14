cmake_minimum_required(VERSION 3.25...3.29)

# Main executable
# ---------------
add_executable(VolumeSplatting main.cpp)
set_target_properties(VolumeSplatting PROPERTIES CXX_STANDARD 23)
torpedo_demo_clang_setup(VolumeSplatting)

# Dependencies
# ------------
target_link_libraries(VolumeSplatting PRIVATE torpedo::volumetric)
target_link_libraries(VolumeSplatting PRIVATE torpedo::extension)

# Download a trained point cloud and put it in the build directory
# ----------------------------------------------------------------
torpedo_demo_assets_dir(VolumeSplatting VOLUME_SPLATTING ${CMAKE_CURRENT_BINARY_DIR} VOLUME_SPLATTING_ASSETS_DIR)
file(DOWNLOAD
    https://github.com/ndming/3d-gaussian-splatting/releases/download/0.0.1/mipnerf360-counter-iter-30000.ply
    ${VOLUME_SPLATTING_ASSETS_DIR}/counter-iter-30000.ply
    EXPECTED_MD5 05759CC5811AE8BEA1D099AE040AEDD8
    SHOW_PROGRESS
)