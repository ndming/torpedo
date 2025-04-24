# MODULE SOURCE FILES
# -------------------
set(TORPEDO_MATH_SOURCES
    include/torpedo/math/mat4.h
    include/torpedo/math/transform.h
    src/vec2.cpp
    src/vec3.cpp
    src/vec4.cpp)


# TORPEDO MATH MODULE
# -------------------
set(TARGET torpedo_math)
add_library(${TARGET} STATIC)
add_library(torpedo::math ALIAS ${TARGET})
target_sources(${TARGET} PRIVATE ${TORPEDO_MATH_SOURCES})
set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD 23
    CMAKE_CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON)

# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)

# Compiler and linker options when compiling with Clang
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (LIBCXX AND LIBABI)
        target_compile_options(${TARGET} PRIVATE -stdlib=libc++)
        target_link_libraries(${TARGET} PRIVATE ${LIBCXX} ${LIBCXXABI})
    endif()
endif()