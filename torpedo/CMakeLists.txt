# TORPEDO BACKEND AND TOOLS
# -------------------------
find_package(Vulkan 1.4 REQUIRED)
find_package(vsg REQUIRED)

# Find GLSL and Slang compilers
message(STATUS "Path to shader compilers:")

find_program(GLSL_COMPILER glslangValidator)
if (GLSL_COMPILER)
    message(STATUS "+ GLSL:  ${GLSL_COMPILER}")
else()
    message(STATUS "+ GLSL: NOT found")
endif()

# Where to look for Slang compiler
find_program(SLANG_COMPILER slangc HINTS ${SLANG_COMPILER_DIR})
if (SLANG_COMPILER)
    message(STATUS "+ Slang: ${SLANG_COMPILER}")
elseif (DEFINED ENV{CONDA_PREFIX})
    message(STATUS "+ Slang: NOT found, did you forget to specify -DSLANG_COMPILER_DIR?")
else()
    message(STATUS "+ Slang: NOT found")
endif()


# UTILITY FUNCTION FOR DEFINING A SUB-PROJECT MODULE
# --------------------------------------------------
function(torpedo_define_module TARGET_NAME MODULE_SOURCES CLANG_SET_VISIBILITY)
    # Working with static libraries
    add_library(${TARGET_NAME} STATIC)

    # Extract and define alias (namespace) target
    string(REPLACE "_" ";" PARTS ${TARGET_NAME}) # Split at '_'
    list(GET PARTS 0 NAMESPACE)
    list(GET PARTS 1 MODULE_NAME)
    add_library("${NAMESPACE}::${MODULE_NAME}" ALIAS ${TARGET_NAME})

    # Set compiler properties and module source files
    target_sources(${TARGET_NAME} PRIVATE ${MODULE_SOURCES})
    set_target_properties(${TARGET_NAME} PROPERTIES
            CXX_STANDARD 23
            CMAKE_CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            COMPILE_WARNING_AS_ERROR ON)

    # Compiler and linker options when compiling with Clang
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if (LIBCXX AND LIBABI)
            # Link to libc++
            target_compile_options(${TARGET_NAME} ${CLANG_SET_VISIBILITY} -stdlib=libc++)
            target_link_libraries(${TARGET_NAME} ${CLANG_SET_VISIBILITY} ${LIBCXX} ${LIBCXXABI})
        elseif (WIN32 AND TORPEDO_MSVCRT_PATH)
            # The current Vulkan-Hpp implementation internally uses strncpy which is marked deprecated for Clang that
            # targets MSVC. This would trigger a warning which is treated as compilation error, so we disable it here
            target_compile_options(${TARGET_NAME} ${CLANG_SET_VISIBILITY} -Wno-deprecated-declarations)
        endif()
    endif()
endfunction()


# UTILITY FUNCTION FOR DEFINING A MACRO PATH TO INTERNAL ASSETS DIR
# -----------------------------------------------------------------
function(torpedo_define_assets_dir TARGET_NAME MODULE_NAME MODULE_BINARY_DIR OUT_DIR_PATH)
    if(CMAKE_CONFIGURATION_TYPES)
        # Multi-configuration generators (Visual Studio, Xcode)
        set(ASSETS_DIR_VALUE "${MODULE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/assets")
    else()
        # Single-configuration generators (Makefiles, Ninja)
        set(ASSETS_DIR_VALUE "${MODULE_BINARY_DIR}/assets")
    endif()
    target_compile_definitions(${TARGET_NAME} PRIVATE TORPEDO_${MODULE_NAME}_ASSETS_DIR=\"${ASSETS_DIR_VALUE}\")
    set(${OUT_DIR_PATH} "${ASSETS_DIR_VALUE}" PARENT_SCOPE)
endfunction()


# UTILITY FUNCTION FOR COMPILING GLSL AS PART OF BUILD
# ----------------------------------------------------
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


# UTILITY FUNCTION FOR COMPILING SLANG AS PART OF BUILD
# ----------------------------------------------------
function(torpedo_compile_slang TARGET_NAME SPIR_V_OUTPUT_DIR SLANG_SOURCE_FILES)
    file(MAKE_DIRECTORY ${SPIR_V_OUTPUT_DIR})

    foreach(SLANG_FILE ${SLANG_SOURCE_FILES})
        get_filename_component(FILE_NAME ${SLANG_FILE} NAME)
        set(SPIR_V "${SPIR_V_OUTPUT_DIR}/${FILE_NAME}.spv")

        add_custom_command(
                OUTPUT ${SPIR_V}
                COMMAND ${SLANG_COMPILER} ${SLANG_FILE} -profile glsl_460 -target spirv -o ${SPIR_V} -entry main
                DEPENDS ${SLANG_FILE}
                COMMENT "Compiling ${FILE_NAME}")

        list(APPEND SPIR_V_BINARY_FILES ${SPIR_V})
    endforeach(SLANG_FILE)

    # Create an INTERFACE library to represent the compiled shaders
    add_library(${TARGET_NAME}_spirv_binaries INTERFACE)
    target_sources(${TARGET_NAME}_spirv_binaries INTERFACE ${SPIR_V_BINARY_FILES})
endfunction()


# SUB-PROJECT MODULES
# -------------------
set(TORPEDO_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(${TORPEDO_SOURCE_DIR}/bootstrap)
add_subdirectory(${TORPEDO_SOURCE_DIR}/foundation)
add_subdirectory(${TORPEDO_SOURCE_DIR}/math)
add_subdirectory(${TORPEDO_SOURCE_DIR}/rendering)
add_subdirectory(${TORPEDO_SOURCE_DIR}/volumetric)
add_subdirectory(${TORPEDO_SOURCE_DIR}/differential)
