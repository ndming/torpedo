# ======================================================================================================================
# LIBARY SOURCE FILES
# ======================================================================================================================
set(TORPEDO_FOUNDATION_SOURCES
        src/AllocationUtils.cpp
        src/Buffer.cpp
        src/DeviceAllocator.cpp
        src/Image.cpp
        src/ShaderLayout.cpp
        src/RingBuffer.cpp
        src/ShaderInstance.cpp
        src/StagingBuffer.cpp
        src/StorageBuffer.cpp
        src/VmaUsage.cpp
)

# ======================================================================================================================
# TORPEDO FOUNDATION LIBARY
# ======================================================================================================================
set(TARGET torpedo_foundation)
add_library(${TARGET} STATIC)
add_library(torpedo::foundation ALIAS ${TARGET})
set_target_properties(${TARGET} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF COMPILE_WARNING_AS_ERROR ON)
target_sources(${TARGET} PRIVATE ${TORPEDO_FOUNDATION_SOURCES})

# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
target_include_directories(${TARGET} PUBLIC ${Vulkan_INCLUDE_DIRS})

# Dependencies
target_link_libraries(${TARGET} PUBLIC ${Vulkan_LIBRARIES})

# ======================================================================================================================
# CLANG SPECIFICS
# ======================================================================================================================
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    torpedo_setup_clang(${TARGET})

    if (WIN32)
        # The VMA library does not use nullability completeness annotations which would trigger warnings,
        # we disable them here for the foundation library only
        target_compile_options(${TARGET} PRIVATE -Wno-nullability-completeness)
    endif ()
endif()