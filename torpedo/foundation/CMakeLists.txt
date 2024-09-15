# ======================================================================================================================
# LIBARY SOURCE FILES
# ======================================================================================================================
set(TORPEDO_FOUNDATION_SOURCES
        src/VmaUsage.cpp
        src/ResourceAllocator.cpp
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
target_include_directories(${TARGET} PRIVATE src)

# Dependencies
target_link_libraries(${TARGET} PUBLIC ${Vulkan_LIBRARIES})