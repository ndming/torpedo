# Main executable
# ---------------
set(TARGET HelloGaussian)
add_executable(${TARGET} main.cpp)

# Dependencies
# ------------
target_link_libraries(${TARGET} PRIVATE torpedo::volumetric)
target_link_libraries(${TARGET} PRIVATE torpedo::utility)