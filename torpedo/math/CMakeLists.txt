# TORPEDO MATH INTERFACE
# ----------------------
set(TARGET torpedo_math)
torpedo_define_interface(${TARGET})
# Where to look for header files
target_include_directories(${TARGET} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
