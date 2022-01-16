# CMakeList.txt to build Shortcircuit XT
#
# Shortcircuit XT began with a not-building snap of the code which was used
# at some point in the SC2 product cycle, so we took a different strategy which was:
#
# 1. Have a libshortcircuit-core basically which contains all the code to be SC
# 2. Expose that in an shortcircuit-headless, shortcircuitpy library and exe
# 3. Rebuild the plugin using that library and JUCE
#
# At least that's the plan. Critically this no longer builds any of the old GUI or
# VST2 code, which we couldn't make work anyway. That code is still in the codebase for
# reference as we port, but we should remove it before we ship Shortcircuit XT
#

cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14 CACHE STRING "Minimum macOS version")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(ShortCircuit VERSION 0.5.0.0 LANGUAGES C CXX ASM)

# Share some information about the  build
message(STATUS "ShortCircuit ${CMAKE_PROJECT_VERSION}")
message(STATUS "Compiler Version is ${CMAKE_CXX_COMPILER_VERSION}")
if (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    message(STATUS "Building in 32 bit configuration")
else ()
    message(STATUS "Building in 64 bit configuration")
endif ()


# Optional variables
option(SCXT_INCLUDE_ASIO_SUPPORT "Automatically add ASIO support" FALSE)
option(SCXT_SKIP_PYTHON "Skip the Python Wrapper" FALSE)


# Everything here is C++ 17 now
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND UNIX AND NOT APPLE AND NOT SURGE_SKIP_PIE_CHANGE)
    message(STATUS "Setting -no-pie on EXE flags; use SURGE_SKIP_PIE_CHANGE=TRUE to avoid")
    set(CMAKE_EXE_LINKER_FLAGS "-no-pie")
endif ()

include(cmake/compiler-and-git.cmake)

add_subdirectory(libs)
add_subdirectory(src)
add_subdirectory(wrappers)
add_subdirectory(tests)

include(cmake/code-quality.cmake)
include(cmake/basic-installer.cmake)