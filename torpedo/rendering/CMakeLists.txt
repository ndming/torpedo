# 3RD-PARTY DEPENDENCIES
# ----------------------
include(FetchContent)

# For Window management
FetchContent_Declare(
    glfw GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4 GIT_SHALLOW ON)

# For debug logging
FetchContent_Declare(
    plog GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git
    GIT_TAG 495a54de43e21aaf74b7f2704297aeeae16da421)

FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(plog)


# MODULE SOURCE FILES
# -------------------
set(TORPEDO_RENDERING_SOURCES
        src/Context.cpp
        src/Engine.cpp
        src/HeadlessRenderer.cpp
        src/LogUtils.cpp
        src/Renderer.cpp
        src/SurfaceRenderer.cpp
        src/TransferWorker.cpp)


# TORPEDO RENDERING MODULE
# ------------------------
set(TARGET torpedo_rendering)
torpedo_define_module(${TARGET} "${TORPEDO_RENDERING_SOURCES}" PRIVATE)
# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
# Dependencies
target_link_libraries(${TARGET} PUBLIC torpedo::bootstrap)
target_link_libraries(${TARGET} PUBLIC torpedo::foundation)
target_link_libraries(${TARGET} PUBLIC glfw)
target_link_libraries(${TARGET} PUBLIC plog::plog)


# MACRO DEFINITIONS FOR GLFW BUILD ON LINUX/WSL
# ---------------------------------------------
if (LINUX)
    target_compile_definitions(glfw PRIVATE _XOPEN_SOURCE=700)
    target_compile_definitions(glfw PRIVATE _POSIX_C_SOURCE=199309L)
endif()
