# MODULE SOURCE FILES
# -------------------
set(TORPEDO_VOLUMETRIC_SOURCES
        src/GaussianEngine.cpp)


# TORPEDO VOLUMETRIC MODULE
# ------------------------
set(TARGET torpedo_volumetric)
torpedo_define_module(${TARGET} "${TORPEDO_VOLUMETRIC_SOURCES}")
# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
# Dependencies
target_link_libraries(${TARGET} PUBLIC torpedo::rendering)


# CLANG SPECIFICS
# ---------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    torpedo_setup_clang(${TARGET} PUBLIC)
endif()