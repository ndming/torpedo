cmake_minimum_required(VERSION 3.25...3.29)

include(cmake/BuildOptions.cmake)
include(cmake/Toolchains.cmake)


# PROJECT DECLARATION
# -------------------
project(torpedo VERSION 0.0.0 LANGUAGES C CXX)


# SUB-PROJECTS
# ------------
set(TORPEDO_PROJECT_ROOT ${CMAKE_SOURCE_DIR})

# Core library
add_subdirectory(${TORPEDO_PROJECT_ROOT}/torpedo)

# Demo targets
if (TORPEDO_BUILD_DEMO)
    add_subdirectory(${TORPEDO_PROJECT_ROOT}/demo)
endif()

# pedo engine
if (TORPEDO_BUILD_PEDO)
    add_subdirectory(${TORPEDO_PROJECT_ROOT}/pedo)
endif()
