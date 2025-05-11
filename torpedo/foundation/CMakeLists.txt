# MODULE SOURCE FILES
# -------------------
set(TORPEDO_FOUNDATION_SOURCES
        src/Allocation.cpp
        src/Buffer.cpp
        src/Image.cpp
        src/ImageUtils.cpp
        src/ReadbackBuffer.cpp
        src/ShaderLayout.cpp
        src/RingBuffer.cpp
        src/ShaderInstance.cpp
        src/StorageBuffer.cpp
        src/Target.cpp
        src/Texture.cpp
        src/TransferWorker.cpp
        src/VmaUsage.cpp)


# TORPEDO FOUNDATION MODULE
# -------------------------
set(TARGET torpedo_foundation)
torpedo_define_module(${TARGET} "${TORPEDO_FOUNDATION_SOURCES}")
# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
target_include_directories(${TARGET} PUBLIC ${Vulkan_INCLUDE_DIRS})
# Dependencies
target_link_libraries(${TARGET} PUBLIC ${Vulkan_LIBRARIES})


# Some issues with VMA
# --------------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # The VMA library does not use nullability completeness annotations
    target_compile_options(${TARGET} PRIVATE -Wno-nullability-completeness)
endif()

if(DEFINED ENV{CONDA_PREFIX})
    # The way the VMA header is included is different on Linux without using a Conda environment
    # This macro will tell us if the module is being built in a Conda environment
    target_compile_definitions(${TARGET} PRIVATE TORPEDO_CONDA_BUILD)
endif()