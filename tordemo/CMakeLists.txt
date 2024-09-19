cmake_minimum_required(VERSION 3.22...3.30)

include(FetchContent)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1)
FetchContent_Declare(
        plog
        GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
        GIT_TAG 1.1.10)

FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(glm)
FetchContent_MakeAvailable(plog)

set(TORDEMO_SOURCE_FILES
        src/HelloTriangle.cpp
        src/TordemoApplication.cpp
        main.cpp)

add_executable(tordemo ${TORDEMO_SOURCE_FILES})
target_include_directories(tordemo PRIVATE src)
set_target_properties(tordemo PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF COMPILE_WARNING_AS_ERROR ON)

# Dependencies
target_link_libraries(tordemo PRIVATE torpedo::core)
target_link_libraries(tordemo PRIVATE torpedo::foundation)
target_link_libraries(tordemo PRIVATE glfw)
target_link_libraries(tordemo PRIVATE glm::glm)
target_link_libraries(tordemo PRIVATE plog::plog)

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
)
compile_glsl(tordemo ${TORDEMO_SHADER_OUTPUT_DIR} "${TORDEMO_SHADER_FILES}")
target_link_libraries(tordemo PRIVATE tordemo_spirv_binaries)
