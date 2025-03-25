# GENERAL COMPILER FLAGS
# ----------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# TORPEDO BACKEND AND TOOLS
# -------------------------
find_package(Vulkan 1.4 REQUIRED)


# UTILITY FUNCTION FOR DEFINING A SUB-PROJECT MODULE
# --------------------------------------------------
function(torpedo_define_module TARGET_NAME MODULE_SOURCES)
    # Working with static libraries
    add_library(${TARGET_NAME} STATIC)

    # Extract and define alias (namespace) target
    string(REPLACE "_" ";" PARTS ${TARGET_NAME}) # Split at '_'
    list(GET PARTS 0 NAMESPACE)
    list(GET PARTS 1 MODULE_NAME)
    add_library("${NAMESPACE}::${MODULE_NAME}" ALIAS ${TARGET_NAME})

    # Set compiler options and module sources
    set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF COMPILE_WARNING_AS_ERROR ON)
    target_sources(${TARGET_NAME} PRIVATE ${MODULE_SOURCES})
endfunction()


# UTILITY FUNCTION FOR DEFINING A MACRO PATH TO INTERNAL ASSETS DIR
# -----------------------------------------------------------------
function(torpedo_define_assets_dir TARGET_NAME MODULE_NAME MODULE_BINARY_DIR)
    if(CMAKE_CONFIGURATION_TYPES)
        # Multi-configuration generators (Visual Studio, Xcode)
        set(ASSETS_DIR_VALUE "${MODULE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/assets")
    else()
        # Single-configuration generators (Makefiles, Ninja)
        set(ASSETS_DIR_VALUE "${MODULE_BINARY_DIR}/assets")
    endif()
    target_compile_definitions(${TARGET_NAME} PRIVATE TORPEDO_${MODULE_NAME}_ASSETS_DIR=\"${ASSETS_DIR_VALUE}\")
endfunction()


# UTILITY FUNCTION FOR COMPILING GLSL AS PART OF BUILD
# ----------------------------------------------------
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


# UTILITY FUNCTION FOR CLANG OPTION AND LINKING SETUP
# ---------------------------------------------------
function(torpedo_setup_clang TARGET_NAME VISIBILITY)
    if (WIN32)
        # The current Vulkan-Hpp implementation internally uses strncpy which is marked deprecated for Clang on Windows.
        # This would trigger a warning which is treated as compilation error, so we disable it here
        target_compile_options(${TARGET_NAME} ${VISIBILITY} -Wno-deprecated-declarations)
    else ()
        # Use libc++ for Clang on non-Windows platforms
        target_compile_options(${TARGET_NAME} ${VISIBILITY} -stdlib=libc++)
        target_link_libraries(${TARGET_NAME} ${VISIBILITY} :libc++.a :libc++abi.a)
    endif ()
endfunction()


# SUB-PROJECT MODULES
# -------------------
set(TORPEDO_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(${TORPEDO_SOURCE_DIR}/bootstrap)
add_subdirectory(${TORPEDO_SOURCE_DIR}/foundation)
add_subdirectory(${TORPEDO_SOURCE_DIR}/rendering)
add_subdirectory(${TORPEDO_SOURCE_DIR}/volumetric)
add_subdirectory(${TORPEDO_SOURCE_DIR}/differential)
add_subdirectory(${TORPEDO_SOURCE_DIR}/utility)
