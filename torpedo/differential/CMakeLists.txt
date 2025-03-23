# MODULE SOURCE FILES
# -------------------
set(TORPEDO_DIFFERENTIAL_SOURCES
        src/GaussianRasterizer.cpp)


# TORPEDO BOOTSTRAP MODULE
# ------------------------
set(TARGET torpedo_differential)
torpedo_define_module(${TARGET} "${TORPEDO_DIFFERENTIAL_SOURCES}")
# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
# Dependencies
target_link_libraries(${TARGET} PUBLIC torpedo::rendering)