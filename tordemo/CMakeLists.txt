cmake_minimum_required(VERSION 3.22...3.30)

include(FetchContent)

FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.4.3)
FetchContent_MakeAvailable(assimp)

set(TORDEMO_SOURCE_FILES
#        src/HelloTriangle.cpp
#        src/ModelViewer.cpp
#        src/TexturedCube.cpp
#        src/TordemoApplication.cpp
        ext/stb.cpp
        main.cpp)

add_executable(tordemo ${TORDEMO_SOURCE_FILES})
target_include_directories(tordemo PRIVATE src ext)
set_target_properties(tordemo PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF COMPILE_WARNING_AS_ERROR ON)

# Dependencies
target_link_libraries(tordemo PRIVATE torpedo::graphics)
target_link_libraries(tordemo PRIVATE assimp::assimp)

# Compiler options for Clang
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if (WIN32)
        target_compile_options(tordemo PRIVATE -Wno-deprecated-declarations)
    else ()
        target_compile_options(tordemo PRIVATE -stdlib=libc++)
        target_link_libraries(tordemo PRIVATE :libc++.a :libc++abi.a)
    endif ()
endif()

# Set the path to assets folder
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-configuration generators (Visual Studio, Xcode)
    set(TORDEMO_ASSETS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/assets")
else()
    # Single-configuration generators (Makefiles, Ninja)
    set(TORDEMO_ASSETS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")
endif()

# Shader assets
set(TORDEMO_SHADER_OUTPUT_DIR "${TORDEMO_ASSETS_OUTPUT_DIR}/shaders")
set(TORDEMO_SHADER_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/blank.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/blank.frag
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/simple.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/simple.frag
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/textured.vert
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/textured.frag
)
compile_glsl(tordemo ${TORDEMO_SHADER_OUTPUT_DIR} "${TORDEMO_SHADER_FILES}")
target_link_libraries(tordemo PRIVATE tordemo_spirv_binaries)

function(copy_resource RESOURCE_TYPE RESOURCE_OUTPUT_DIR RESOURCE_FILES)
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
    add_custom_target(copy_${RESOURCE_TYPE} ALL DEPENDS ${RESOURCE_DST_FILES})
    add_dependencies(tordemo copy_${RESOURCE_TYPE})
endfunction()

# Texture assets
set(TORDEMO_TEXTURE_OUTPUT_DIR "${TORDEMO_ASSETS_OUTPUT_DIR}/textures")
set(TORDEMO_TEXTURE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/container_diffuse.png
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/container_specular.png
)
copy_resource("texture" ${TORDEMO_TEXTURE_OUTPUT_DIR} "${TORDEMO_TEXTURE_FILES}")
