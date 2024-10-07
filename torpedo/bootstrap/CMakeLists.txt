# ======================================================================================================================
# LIBARY SOURCE FILES
# ======================================================================================================================
set(TORPEDO_BOOTSTRAP_SOURCES
        src/DeviceBuilder.cpp
        src/InstanceBuilder.cpp
        src/PhysicalDeviceSelector.cpp
        src/SamplerBuilder.cpp
)


# ======================================================================================================================
# TORPEDO BOOTSTRAP LIBARY
# ======================================================================================================================
set(TARGET torpedo_bootstrap)
add_library(${TARGET} STATIC)
add_library(torpedo::bootstrap ALIAS ${TARGET})
set_target_properties(${TARGET} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF COMPILE_WARNING_AS_ERROR ON)
target_sources(${TARGET} PRIVATE ${TORPEDO_BOOTSTRAP_SOURCES})

# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
target_include_directories(${TARGET} PUBLIC ${Vulkan_INCLUDE_DIRS})

# Dependencies
target_link_libraries(${TARGET} PUBLIC ${Vulkan_LIBRARIES})

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
