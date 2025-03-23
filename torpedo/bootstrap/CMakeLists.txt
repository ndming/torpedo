# =================== #
# MODULE SOURCE FILES #
# =================== #
set(TORPEDO_BOOTSTRAP_SOURCES
        src/DebugUtils.cpp
        src/DeviceBuilder.cpp
        src/InstanceBuilder.cpp
        src/PhysicalDeviceSelector.cpp
        src/SamplerBuilder.cpp
        src/SwapChainBuilder.cpp
)


# ======================== #
# TORPEDO BOOTSTRAP MODULE #
# ======================== #
set(TARGET torpedo_bootstrap)
torpedo_define_module(${TARGET} "${TORPEDO_BOOTSTRAP_SOURCES}")

# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
target_include_directories(${TARGET} PUBLIC ${Vulkan_INCLUDE_DIRS})

# Dependencies
target_link_libraries(${TARGET} PUBLIC ${Vulkan_LIBRARIES})

# =============== #
# CLANG SPECIFICS #
# =============== #
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    torpedo_setup_clang(${TARGET})
endif()
