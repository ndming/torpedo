# MODULE SOURCE FILES
# -------------------
set(TORPEDO_FOUNDATION_SOURCES
        src/Allocation.cpp
        src/Buffer.cpp
        src/DeviceAllocator.cpp
        src/Image.cpp
        src/ImageUtils.cpp
        src/ReadbackBuffer.cpp
        src/ShaderLayout.cpp
        src/RingBuffer.cpp
        src/ShaderInstance.cpp
        src/StorageBuffer.cpp
        src/Sync.cpp
        src/Target.cpp
        src/Texture.cpp
        src/VmaUsage.cpp)


# TORPEDO FOUNDATION MODULE
# -------------------------
set(TARGET torpedo_foundation)
torpedo_define_module(${TARGET} "${TORPEDO_FOUNDATION_SOURCES}" PRIVATE)
# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
target_include_directories(${TARGET} PUBLIC ${Vulkan_INCLUDE_DIRS})
# Dependencies
target_link_libraries(${TARGET} PUBLIC ${Vulkan_LIBRARIES})


# CLANG SPECIFICS
# ---------------
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # The VMA library does not use nullability completeness annotations
    target_compile_options(${TARGET} PRIVATE -Wno-nullability-completeness)
endif()