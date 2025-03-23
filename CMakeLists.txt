cmake_minimum_required(VERSION 3.22...3.29)

# =================== #
# PROJECT DECLARATION #
# =================== #
project(torpedo VERSION 1.0.0 LANGUAGES C CXX)


# ============= #
# BUILD OPTIONS #
# ============= #
option(TORPEDO_BUILD_DEMO "Build Torpedo demo targets" ON)
option(TORPEDO_INSTALL "Generate installation targets" ON)


# ============ #
# SUB-PROJECTS #
# ============ #
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
add_subdirectory(${PROJECT_ROOT}/torpedo)
add_subdirectory(${PROJECT_ROOT}/demo)
