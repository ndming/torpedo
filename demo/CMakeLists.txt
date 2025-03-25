cmake_minimum_required(VERSION 3.22...3.30)

# General compiler flags
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Copy resources to build directory
function(tordemo_copy_resource TARGET_NAME RESOURCE_OUTPUT_DIR RESOURCE_FILES)
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

# tordemo samples
add_subdirectory(HelloTriangle)
# add_subdirectory(PhongCube)
# add_subdirectory(TexturedCube)
# add_subdirectory(ModelLoading)
add_subdirectory(HelloGaussian)
