# Main executable
# ---------------
set(TARGET PhongCube)
add_executable(${TARGET} main.cpp)

# Dependencies
# ------------
target_link_libraries(${TARGET} PRIVATE torpedo::graphics)

# Ony applicable if compiling against Clang
# -----------------------------------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (WIN32)
        # The current Vulkan-Hpp implementation internally uses strncpy which is marked deprecated for Clang on Windows.
        # This would trigger a warning which is treated as compilation error, so we disable it here
        target_compile_options(${TARGET} PRIVATE -Wno-deprecated-declarations)
    else ()
        # Use libc++ for Clang on non-Windows platforms
        target_compile_options(${TARGET} PRIVATE -stdlib=libc++)
        target_link_libraries(${TARGET} PRIVATE :libc++.a :libc++abi.a)
    endif ()
endif()