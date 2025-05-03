# TORPEDO BUILD OPTIONS
# ---------------------
option(TORPEDO_BUILD_DEMO "Build torpedo demo targets" OFF)
option(TORPEDO_BUILD_PEDO "Build pedo Gaussian engine" ON)

# Default to Release build
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Default to Release build" FORCE)
endif()
message(STATUS "Configuring for ${CMAKE_BUILD_TYPE} build:")

# Build demo targets by default for Debug build
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT TORPEDO_BUILD_DEMO)
    set(TORPEDO_BUILD_DEMO ON)
endif()
message(STATUS "+ Build demo: ${TORPEDO_BUILD_DEMO}")
message(STATUS "+ Build pedo: ${TORPEDO_BUILD_PEDO}")


# GLFW SPECIFICS
# --------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Don't build the docs")
set(GLFW_INSTALL    OFF CACHE BOOL "Don't generate install targets")

# Turn off GLFW's X11 support on Linux/WSL for now until we find a solution to build with X11 entirely in conda
if (CMAKE_HOST_LINUX AND DEFINED ENV{CONDA_PREFIX} AND NOT GLFW_BUILD_X11)
    set(GLFW_BUILD_X11 OFF CACHE BOOL "Disable X11 support for GLFW")
    message(STATUS "Disabled X11 support for GLFW")
endif()
