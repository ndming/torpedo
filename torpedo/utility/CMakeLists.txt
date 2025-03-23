# 3RD-PARTY DEPENDENCIES
# ----------------------
include(FetchContent)
# For debug logging
FetchContent_Declare(plog GIT_REPOSITORY https://github.com/SergiusTheBest/plog.git GIT_TAG 1.1.10)
FetchContent_MakeAvailable(plog)


# MODULE SOURCE FILES
# -------------------
set(TORPEDO_UTILS_SOURCES
        src/Log.cpp)


# TORPEDO UTILITY MODULE
# ----------------------
set(TARGET torpedo_utility)
torpedo_define_module(${TARGET} "${TORPEDO_UTILS_SOURCES}")
# Where to look for header files
target_include_directories(${TARGET} PUBLIC include)
# Dependencies
target_link_libraries(${TARGET} PUBLIC plog::plog)
