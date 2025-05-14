# Copy resources to build directory
# ---------------------------------
function(torpedo_demo_copy_resource TARGET_NAME RESOURCE_OUTPUT_DIR RESOURCE_FILES)
    foreach(RESOURCE_FILE ${RESOURCE_FILES})
        # Get the file name from the full path
        get_filename_component(FILE_NAME ${RESOURCE_FILE} NAME)
        set(RESOURCE_DST_FILE "${RESOURCE_OUTPUT_DIR}/${FILE_NAME}")

        # Add a custom command to copy the texture file
        add_custom_command(
                OUTPUT ${RESOURCE_DST_FILE}
                COMMAND ${CMAKE_COMMAND} -E copy ${RESOURCE_FILE} ${RESOURCE_DST_FILE}
                DEPENDS ${RESOURCE_FILE}
                COMMENT "Copying ${FILE_NAME} to build directory")

        # Collect the list of destination files
        list(APPEND RESOURCE_DST_FILES ${RESOURCE_DST_FILE})
    endforeach()
    string(RANDOM LENGTH 6 ALPHABET "0123456789abcdef" suffix)
    add_custom_target(${TARGET_NAME}_resource_copy_${suffix} ALL DEPENDS ${RESOURCE_DST_FILES})
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_resource_copy_${suffix})
endfunction()

# Clang specifics
# ---------------
function(torpedo_demo_clang_setup TARGET_NAME)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if (TORPEDO_LIBCXX_PATH AND TORPEDO_LIBABI_PATH)
            # Link to libc++
            target_compile_options(${TARGET_NAME} PRIVATE -stdlib=libc++)
            target_link_libraries(${TARGET_NAME} PRIVATE ${TORPEDO_LIBCXX_PATH} ${TORPEDO_LIBABI_PATH})
        elseif (WIN32 AND TORPEDO_MSVCRT_PATH)
            # The current Vulkan-Hpp implementation internally uses strncpy which is marked deprecated for Clang that
            # targets MSVC. This would trigger a warning which is treated as compilation error, so we disable it here
            target_compile_options(${TARGET_NAME} PRIVATE -Wno-deprecated-declarations)
        endif()
    endif()
endfunction()

# Call this to receive a macro pointing to the the assets directory
# -----------------------------------------------------------------
function(torpedo_demo_assets_dir TARGET_NAME MODULE_NAME TARGET_BINARY_DIR OUT_DIR_PATH)
    if(CMAKE_CONFIGURATION_TYPES)
        # Multi-configuration generators (Visual Studio, Xcode)
        set(ASSETS_DIR_VALUE "${TARGET_BINARY_DIR}/${CMAKE_CFG_INTDIR}/assets")
    else()
        # Single-configuration generators (Makefiles, Ninja)
        set(ASSETS_DIR_VALUE "${TARGET_BINARY_DIR}/assets")
    endif()
    target_compile_definitions(${TARGET_NAME} PRIVATE ${MODULE_NAME}_ASSETS_DIR=\"${ASSETS_DIR_VALUE}\")
    set(${OUT_DIR_PATH} "${ASSETS_DIR_VALUE}" PARENT_SCOPE)
endfunction()

# Demo targets
add_subdirectory(HelloGaussian)
add_subdirectory(VolumeSplatting)
