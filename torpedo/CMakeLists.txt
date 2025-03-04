# ======================================================================================================================
# GENERAL COMPILER FLAGS
# ======================================================================================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# ======================================================================================================================
# TORPEDO BACKEND
# ======================================================================================================================
find_package(Vulkan REQUIRED)


# ======================================================================================================================
# UTILITY FUNCTION FOR COMPILING GLSL SOURCES AS PART OF BUILD
# ======================================================================================================================

# Where to look for the compiler
find_program(GLSL_COMPILER glslc HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
message(STATUS "Using GLSL compiler at: ${GLSL_COMPILER}")

# GLSL compilation
function(torpedo_compile_glsl TARGET_NAME SPIR_V_OUTPUT_DIR GLSL_SOURCE_FILES)
    file(MAKE_DIRECTORY ${SPIR_V_OUTPUT_DIR})

    foreach(GLSL_FILE ${GLSL_SOURCE_FILES})
        get_filename_component(FILE_NAME ${GLSL_FILE} NAME)
        set(SPIR_V "${SPIR_V_OUTPUT_DIR}/${FILE_NAME}.spv")

        add_custom_command(
                OUTPUT ${SPIR_V}
                COMMAND ${GLSL_COMPILER} -o ${SPIR_V} ${GLSL_FILE}
                DEPENDS ${GLSL_FILE}
                COMMENT "Compiling ${FILE_NAME}")

        list(APPEND SPIR_V_BINARY_FILES ${SPIR_V})
    endforeach(GLSL_FILE)

    # Create an INTERFACE library to represent the compiled shaders
    add_library(${TARGET_NAME}_spirv_binaries INTERFACE)
    target_sources(${TARGET_NAME}_spirv_binaries INTERFACE ${SPIR_V_BINARY_FILES})
endfunction()


# ======================================================================================================================
# CLANG UTILS
# ======================================================================================================================

# If a target is compiled against Clang, this utility function can help set up correct compilation and linking options
function(torpedo_setup_clang TARGET_NAME)
    if (WIN32)
        # The current Vulkan-Hpp implementation internally uses strncpy which is marked deprecated for Clang on Windows.
        # This would trigger a warning which is treated as compilation error, so we disable it here
        target_compile_options(${TARGET_NAME} PRIVATE -Wno-deprecated-declarations)
    else ()
        # Use libc++ for Clang on non-Windows platforms
        target_compile_options(${TARGET_NAME} PRIVATE -stdlib=libc++)
        target_link_libraries(${TARGET_NAME} PRIVATE :libc++.a :libc++abi.a)
    endif ()
endfunction()


# ======================================================================================================================
# SUB-PROJECT MODULES
# ======================================================================================================================
set(TORPEDO_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(${TORPEDO_SOURCE_DIR}/bootstrap)
add_subdirectory(${TORPEDO_SOURCE_DIR}/foundation)
add_subdirectory(${TORPEDO_SOURCE_DIR}/rendering)
add_subdirectory(${TORPEDO_SOURCE_DIR}/utils)
