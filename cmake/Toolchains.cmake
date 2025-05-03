# TOOLCHAIN MANAGEMENT
# --------------------
# Help CMake find relevant compiler, linker, libraries, and system utilities based on host OS and active environments.
# For each OS, there are two build patterns to consider: system-wide build and conda build. The toolchain file will
# detect these pattern based on certain assumptions about the environment and emit the following variables accordingly:
# - TORPEDO_LIBCXX_PATH: path to LLVM C++ Standard Library, useful for Unix-based and Cygwin systems
# - TORPEDO_LIBABI_PATH: path to LLVM low-level C++ ABI, useful for Unix-based and Cygwin systems
# - TORPEDO_MSVCRT_PATH: path to the MSVC runtime components, useful for Windows systems

# First find the path to the MSVC runtime components on Windows, if any.
# Note that CMAKE_HOST_WIN32 also includes MinGW.
if (CMAKE_HOST_WIN32)
    # Refer to: https://izzys.casa/2023/09/finding-msvc-with-cmake/
    # Block was added in CMake 3.25, and lets us create variable scopes without using a function.
    block(SCOPE_FOR VARIABLES)
        cmake_path(
            CONVERT "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/Installer"
            TO_CMAKE_PATH_LIST vswhere.dir
            NORMALIZE)
        # This only temporarily affects the variable since we're inside a block.
        list(APPEND CMAKE_SYSTEM_PROGRAM_PATH "${vswhere.dir}")
        find_program(VSWHERE_EXECUTABLE NAMES vswhere DOC "Visual Studio Locator")
    endblock()
    # Found vswhere.exe
    if (VSWHERE_EXECUTABLE)
        execute_process(COMMAND "${VSWHERE_EXECUTABLE}" -nologo -nocolor
            -format json
            -products "*"
            -utf8
            -sort
            ENCODING UTF-8
            OUTPUT_VARIABLE candidates
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(JSON candidates.length LENGTH "${candidates}")
        # Found working MSVC installations, pick the first one
        if (candidates.length GREATER 0)
            string(JSON candidate.install.path GET "${candidates}" 0 "installationPath")
            if (candidate.install.path)
                cmake_path(CONVERT "${candidate.install.path}" TO_CMAKE_PATH_LIST candidate.install.path NORMALIZE)
                # Pick the latest version
                file(GLOB versions LIST_DIRECTORIES YES
                    RELATIVE "${candidate.install.path}/VC/Tools/MSVC"
                    "${candidate.install.path}/VC/Tools/MSVC/*")
                list(SORT versions COMPARE NATURAL ORDER DESCENDING)
                list(GET  versions 0 version.dir)
                # Set TORPEDO_MSVCRT_PATH to the found msvcrt.lib
                find_library(MSVCRT NAMES msvcrt PATHS "${candidate.install.path}/VC/Tools/MSVC/${version.dir}/lib/x64" NO_CACHE)
                set(TORPEDO_MSVCRT_PATH ${MSVCRT} CACHE INTERNAL "Path to MSVC runtime components")
            endif()
        endif()
    endif()
endif()

# Check if we're currently inside a conda environment
if (DEFINED ENV{CONDA_PREFIX})
    message(STATUS "Detected a conda environment:")

    # Find if the environment has libc++ and libc++abi
    if (CMAKE_HOST_UNIX)  # Linux, macOS, Cygwin
        find_library(LIBCXX NAMES c++    PATHS $ENV{CONDA_PREFIX}/lib NO_CACHE)
        find_library(LIBABI NAMES c++abi PATHS $ENV{CONDA_PREFIX}/lib NO_CACHE)
    else()                # assuming win64 and MinGW
        find_library(LIBCXX NAMES c++    PATHS $ENV{CONDA_PREFIX}/Library/lib NO_CACHE)
        find_library(LIBABI NAMES c++abi PATHS $ENV{CONDA_PREFIX}/Library/lib NO_CACHE)
    endif()

    # Report the path to libc++ if found
    if (LIBCXX AND LIBABI)
        set(TORPEDO_LIBCXX_PATH ${LIBCXX} CACHE INTERNAL "Path to libc++ in conda environment")
        set(TORPEDO_LIBABI_PATH ${LIBABI} CACHE INTERNAL "Path to libc++abi in conda environment")
        message(STATUS "+ Path to libc++: ${LIBCXX}")
    # If not, flag an error on Unix-based systems since it would only take an extra conda package for libc++
    elseif (UNIX)
        string(JOIN " " error "Could NOT find libc++/libc++abi in the current conda environment at: $ENV{CONDA_PREFIX}/lib."
                              "PLease refer to the build instructions in README for working with the library inside conda.")
        message(FATAL_ERROR ${error})
    # On Windows, however, we can still link against msvcrt.lib by having clang target msvc, provided we found one
    elseif (TORPEDO_MSVCRT_PATH)
        message(STATUS "+ Path to msvcrt: ${TORPEDO_MSVCRT_PATH}")
    endif()

    # Automatically set install prefix to the current conda environment if no prefix has been set
    if (NOT CMAKE_INSTALL_PREFIX)
        if (CMAKE_HOST_UNIX)  # Linux, macOS, Cygwin
            set(CMAKE_INSTALL_PREFIX $ENV{CONDA_PREFIX} CACHE PATH "Default install prefix to CONDA_PREFIX")
        else()                # assuming win64 and MinGW
            set(CMAKE_INSTALL_PREFIX "$ENV{CONDA_PREFIX}/Library" CACHE PATH "Default install prefix to CONDA_PREFIX")
        endif()
    endif()
    message(STATUS "+ Install prefix: ${CMAKE_INSTALL_PREFIX}")

# When not inside a conda environment, we resort to the default search path for libc++/libc++abi on Unix/Cygwin systems.
# On Windows, CMake knows how to find MSVC runtime components, and we don't have to report the path in these cases.
elseif (CMAKE_HOST_UNIX)
    set(TORPEDO_LIBCXX_PATH :libc++.a    CACHE INTERNAL "Default search path for libc++")
    set(TORPEDO_LIBABI_PATH :libc++abi.a CACHE INTERNAL "Default search path for libc++abi")
endif()

# If no library components found, back out immediately
if (NOT TORPEDO_LIBCXX_PATH AND NOT TORPEDO_LIBABI_PATH AND NOT TORPEDO_MSVCRT_PATH)
    message(FATAL_ERROR "Could not find any standard C++ library components, please refer to the build instructions in README.")
endif()