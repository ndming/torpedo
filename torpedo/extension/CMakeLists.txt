# MODULE SOURCE FILES
# -------------------
set(TORPEDO_EXTENSION_SOURCES
        src/PerspectiveCamera.cpp)

# TORPEDO EXTENSION MODULE
# ------------------------
set(TARGET torpedo_extension)
torpedo_define_module(${TARGET} "${TORPEDO_EXTENSION_SOURCES}")
# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
# Dependencies
target_link_libraries(${TARGET} PUBLIC torpedo::rendering)