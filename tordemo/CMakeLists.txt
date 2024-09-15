cmake_minimum_required(VERSION 3.15...3.30)

include(FetchContent)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4)
FetchContent_Declare(
        plog
        GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
        GIT_TAG 1.1.10)

FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(plog)

set(TORDEMO_SOURCE_FILES
        src/TordemoApplication.cpp
        main.cpp)

add_executable(tordemo ${TORDEMO_SOURCE_FILES})
target_include_directories(tordemo PRIVATE src)
set_target_properties(tordemo PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF COMPILE_WARNING_AS_ERROR ON)

# Dependencies
target_link_libraries(tordemo PRIVATE torpedo::core)
target_link_libraries(tordemo PRIVATE torpedo::foundation)
target_link_libraries(tordemo PRIVATE glfw)
target_link_libraries(tordemo PRIVATE plog::plog)