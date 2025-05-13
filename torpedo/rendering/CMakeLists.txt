# MODULE SOURCE FILES
# -------------------
set(TORPEDO_RENDERING_SOURCES
        src/Camera.cpp
        src/Context.cpp
        src/Control.cpp
        src/Engine.cpp
        src/HeadlessRenderer.cpp
        src/LogUtils.cpp
        src/Renderer.cpp
        src/Scene.cpp
        src/SurfaceRenderer.cpp)


# TORPEDO RENDERING MODULE
# ------------------------
set(TARGET torpedo_rendering)
torpedo_define_module(${TARGET} "${TORPEDO_RENDERING_SOURCES}")
# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
# Dependencies
target_link_libraries(${TARGET} PUBLIC torpedo::bootstrap)
target_link_libraries(${TARGET} PUBLIC torpedo::foundation)
target_link_libraries(${TARGET} PUBLIC torpedo::math)
target_link_libraries(${TARGET} PUBLIC glfw)
target_link_libraries(${TARGET} PUBLIC EnTT::EnTT)
target_link_libraries(${TARGET} PUBLIC plog::plog)


# MACRO DEFINITIONS FOR GLFW BUILD ON LINUX/WSL
# ---------------------------------------------
if (LINUX)
    target_compile_definitions(glfw PRIVATE _XOPEN_SOURCE=700)
    target_compile_definitions(glfw PRIVATE _POSIX_C_SOURCE=199309L)
endif()
