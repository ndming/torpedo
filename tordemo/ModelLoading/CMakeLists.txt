include(FetchContent)

FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.4.3)
FetchContent_MakeAvailable(assimp)

# Main executable
# ---------------
set(TARGET ModelLoading)
add_executable(${TARGET} main.cpp ModelLoader.cpp ext/stb.cpp)
target_include_directories(${TARGET} PRIVATE ext)

# Dependencies
# ------------
target_link_libraries(${TARGET} PRIVATE torpedo::rendering)
target_link_libraries(${TARGET} PRIVATE assimp::assimp)

# Where to look for model files
# ------------------------
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-configuration generators (Visual Studio, Xcode)
    set(MODEL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/models")
else()
    # Single-configuration generators (Makefiles, Ninja)
    set(MODEL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/models")
endif()

# Copy model assets to build directory
# ------------------------------------
set(BACKPACK_MODEL_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/models/backpack/ao.jpg
        ${CMAKE_CURRENT_SOURCE_DIR}/models/backpack/backpack.mtl
        ${CMAKE_CURRENT_SOURCE_DIR}/models/backpack/backpack.obj
        ${CMAKE_CURRENT_SOURCE_DIR}/models/backpack/diffuse.jpg
        ${CMAKE_CURRENT_SOURCE_DIR}/models/backpack/normal.png
        ${CMAKE_CURRENT_SOURCE_DIR}/models/backpack/roughness.jpg
        ${CMAKE_CURRENT_SOURCE_DIR}/models/backpack/specular.jpg
)
tordemo_copy_resource(${TARGET} "${MODEL_OUTPUT_DIR}/backpack" "${BACKPACK_MODEL_FILES}")

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
