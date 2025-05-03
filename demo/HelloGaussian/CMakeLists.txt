cmake_minimum_required(VERSION 3.25...3.29)

# Main executable
# ---------------
add_executable(HelloGaussian main.cpp)
set_target_properties(HelloGaussian PROPERTIES CXX_STANDARD 23)
torpedo_demo_clang_setup(HelloGaussian)

# Dependencies
# ------------
target_link_libraries(HelloGaussian PRIVATE torpedo::volumetric)