# ======================================================================================================================
# 3RD-PARTY LIBRARIES
# ======================================================================================================================
include(FetchContent)

# For debug logging
FetchContent_Declare(plog GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git GIT_TAG 1.1.10)
FetchContent_MakeAvailable(plog)

# ======================================================================================================================
# MODULE SOURCE FILES
# ======================================================================================================================
set(TORPEDO_UTILS_SOURCES
        src/Log.cpp
)

# ======================================================================================================================
# TORPEDO UTILS MODULE
# ======================================================================================================================
set(TARGET torpedo_utils)
add_library(${TARGET} STATIC)
add_library(torpedo::utils ALIAS ${TARGET})
set_target_properties(${TARGET} PROPERTIES CXX_STANDARD 23 CXX_EXTENSIONS OFF COMPILE_WARNING_AS_ERROR ON)
target_sources(${TARGET} PRIVATE ${TORPEDO_UTILS_SOURCES})

# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)

# Dependencies
target_link_libraries(${TARGET} PUBLIC plog::plog)
