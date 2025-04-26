# TORPEDO MATH INTERFACE
# ----------------------
set(TARGET torpedo_math)
add_library(${TARGET} INTERFACE)
add_library(torpedo::math ALIAS ${TARGET})
target_compile_features(${TARGET} INTERFACE cxx_std_23)
target_compile_options(${TARGET} INTERFACE -Werror $<$<CXX_COMPILER_ID:MSVC>:/WX>)

# Where to look for header files
target_include_directories(${TARGET} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Compiler and linker options when compiling with Clang
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (LIBCXX AND LIBABI)
        target_compile_options(${TARGET} INTERFACE -stdlib=libc++)
        target_link_libraries(${TARGET} INTERFACE ${LIBCXX} ${LIBCXXABI})
    endif()
endif()