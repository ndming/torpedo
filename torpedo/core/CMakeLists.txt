# ======================================================================================================================
# LIBARY SOURCE FILES
# ======================================================================================================================
set(TORPEDO_CORE_SOURCES
        src/DeviceBuilder.cpp
        src/InstanceBuilder.cpp
        src/PhysicalDeviceSelector.cpp)


# ======================================================================================================================
# TORPEDO CORE LIBARY
# ======================================================================================================================
set(TARGET torpedo_core)
add_library(${TARGET} STATIC)
add_library(torpedo::core ALIAS ${TARGET})
set_target_properties(${TARGET} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF COMPILE_WARNING_AS_ERROR ON)
target_sources(${TARGET} PRIVATE ${TORPEDO_CORE_SOURCES})

# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
target_include_directories(${TARGET} PUBLIC ${Vulkan_INCLUDE_DIRS})

# Dependencies
target_link_libraries(${TARGET} PUBLIC ${Vulkan_LIBRARIES})
