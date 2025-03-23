# Main executable
# ---------------
set(TARGET HelloGaussian)
add_executable(${TARGET} main.cpp)

# Dependencies
# ------------
target_link_libraries(${TARGET} PRIVATE torpedo::volumetric)
target_link_libraries(${TARGET} PRIVATE torpedo::utility)

# Ony applicable if compiling against Clang
# -----------------------------------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    torpedo_setup_clang(${TARGET})
endif()