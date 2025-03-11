# ======================================================================================================================
# 3RD-PARTY LIBRARIES
# ======================================================================================================================
include(FetchContent)

# For Windowing context
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
FetchContent_MakeAvailable(glfw)

# For debug logging
FetchContent_Declare(plog GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git GIT_TAG 1.1.10)
FetchContent_MakeAvailable(plog)

# ======================================================================================================================
# MODULE SOURCE FILES
# ======================================================================================================================
set(TORPEDO_RENDERING_SOURCES
        src/Engine.cpp
        src/ForwardEngine.cpp
        src/HeadlessRenderer.cpp
        src/Renderer.cpp
        src/StandardRenderer.cpp
        src/Utils.cpp
)


# ======================================================================================================================
# TORPEDO RENDERING MODULE
# ======================================================================================================================
set(TARGET torpedo_rendering)
add_library(${TARGET} STATIC)
add_library(torpedo::rendering ALIAS ${TARGET})
set_target_properties(${TARGET} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF COMPILE_WARNING_AS_ERROR ON)
target_sources(${TARGET} PRIVATE ${TORPEDO_RENDERING_SOURCES})

# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)

# Dependencies
target_link_libraries(${TARGET} PUBLIC torpedo::bootstrap)
target_link_libraries(${TARGET} PUBLIC torpedo::foundation)
target_link_libraries(${TARGET} PUBLIC glfw)
target_link_libraries(${TARGET} PUBLIC plog::plog)

# ======================================================================================================================
# CLANG SPECIFICS
# ======================================================================================================================
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    torpedo_setup_clang(${TARGET})
endif()