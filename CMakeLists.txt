cmake_minimum_required(VERSION 3.22...3.29)


# BUILD OPTIONS
# -------------
option(TORPEDO_BUILD_DEMO "Build torpedo demo targets" OFF)
option(TORPEDO_BUILD_PEDO "Build pedo Gaussian engine" ON)

# Default to Release build
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Default to Release build" FORCE)
endif()
message(STATUS "Configuring for ${CMAKE_BUILD_TYPE} build")

# Build demo targets by default for Debug build
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT TORPEDO_BUILD_DEMO)
    set(TORPEDO_BUILD_DEMO ON)
endif()
message(STATUS "Build demo: ${TORPEDO_BUILD_DEMO}")
message(STATUS "Build pedo: ${TORPEDO_BUILD_PEDO}")

# Modify the install prefix to favor the local conda environment
if(DEFINED ENV{CONDA_PREFIX} AND NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX $ENV{CONDA_PREFIX} CACHE PATH "Default install prefix set to CONDA_PREFIX")
endif()
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")

# Turn off GLFW's X11 support on Linux/WSL
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR DEFINED ENV{WSL_DISTRO_NAME})
    set(GLFW_BUILD_X11 OFF CACHE BOOL "Disable X11 support for GLFW")
    message(STATUS "Disabled GLFW's X11 support")
endif ()

# Common GLFW options
set(GLFW_VULKAN_STATIC ON  CACHE BOOL "Don't dynamically link to the Vulkan loader")
set(GLFW_BUILD_DOCS    OFF CACHE BOOL "Don't build the docs")
set(GLFW_INSTALL       OFF CACHE BOOL "Don't generate install targets")

# PROJECT DECLARATION
# -------------------
project(torpedo VERSION 0.0.0 LANGUAGES C CXX)


# SUB-PROJECTS
# ------------
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
add_subdirectory(${PROJECT_ROOT}/torpedo)
if (TORPEDO_BUILD_DEMO)
    add_subdirectory(${PROJECT_ROOT}/demo)
endif()
