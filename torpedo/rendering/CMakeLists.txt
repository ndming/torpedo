# ====================== #
# 3RD-PARTY DEPENDENCIES #
# ====================== #
include(FetchContent)

# For Window management
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
FetchContent_MakeAvailable(glfw)

# For debug logging
FetchContent_Declare(plog GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git GIT_TAG 1.1.10)
FetchContent_MakeAvailable(plog)

# =================== #
# MODULE SOURCE FILES #
# =================== #
set(TORPEDO_RENDERING_SOURCES
        src/Context.cpp
        src/DeletionWorker.cpp
        src/Engine.cpp
        src/ForwardEngine.cpp
        src/HeadlessRenderer.cpp
        src/Renderer.cpp
        src/SurfaceRenderer.cpp
        src/SyncGroup.cpp
        src/Utils.cpp
)

# ======================== #
# TORPEDO RENDERING MODULE #
# ======================== #
set(TARGET torpedo_rendering)
torpedo_define_module(${TARGET} "${TORPEDO_RENDERING_SOURCES}")

# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)

# Dependencies
target_link_libraries(${TARGET} PUBLIC torpedo::bootstrap)
target_link_libraries(${TARGET} PUBLIC torpedo::foundation)
target_link_libraries(${TARGET} PUBLIC glfw)
target_link_libraries(${TARGET} PUBLIC plog::plog)

# =============== #
# CLANG SPECIFICS #
# =============== #
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    torpedo_setup_clang(${TARGET})
endif()