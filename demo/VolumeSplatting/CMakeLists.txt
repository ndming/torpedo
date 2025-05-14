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
    https://github.com/ndming/3d-gaussian-splatting/releases/download/0.0.1/mipnerf360-bicycle-iter-30000.ply
    ${VOLUME_SPLATTING_ASSETS_DIR}/bicycle-iter-30000.ply
    EXPECTED_MD5 f6d3b2d6133515d49d39a9881f54b6f2
    SHOW_PROGRESS
)

file(DOWNLOAD
    https://github.com/ndming/3d-gaussian-splatting/releases/download/0.0.1/dtu-scan24-iter-30000.ply
    ${VOLUME_SPLATTING_ASSETS_DIR}/scan24-iter-30000.ply
    EXPECTED_MD5 9950f0f1904ccc9f0a0915f5e9acae8c
    SHOW_PROGRESS
)