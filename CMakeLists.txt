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

cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0063 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# https://github.com/taocpp/PEGTL/issues/347 would let us go to 13
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14 CACHE STRING "Minimum macOS version")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO Remove This before we ship non beta version. Remember
# the AU rescans only when versions change even if binaries are
# different.
string(TIMESTAMP DAY_OF_YEAR "%j")
string(TIMESTAMP YEAR "%Y")
math(EXPR PART0 "${YEAR}-2023 + 6")
math(EXPR PART1 "${DAY_OF_YEAR} + 1")

project(ShortcircuitXT VERSION 0.${PART0}.${PART1}.0 LANGUAGES C CXX ASM)
set(ShortcircuitXT_VERSION ${PROJECT_VERSION})
message(STATUS "ShortcircuitXT: Using date-driven version while in alpha - ${PROJECT_VERSION}")

include(libs/sst/sst-plugininfra/cmake/git-version-functions.cmake)
version_from_versionfile_or_git()

# Build Time Options
option(SCXT_BUILD_VST3 "Build a VST3 plugin" ON)
option(SCXT_BUILD_AU "Build an AU plugin" ON)
option(SCXT_BUILD_CLAP "Build a CLAP" ON)
option(SCXT_BUILD_STANDALONE "Build a standalone executable" ON)
option(SCXT_USE_ALSA "Build with ALSA when appropriate" ON)
option(SCXT_USE_JACK "Build with JACK when appropriate" ON)
option(SCXT_COPY_PLUGIN_AFTER_BUILD "Copy plugin after build if possible" OFF)
option(SCXT_SKIP_PIE_CHANGE "Dont add -no-pie to executables" OFF)

if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "arm64ec")
    message(STATUS "Default skipping FLAC and MP3 on arm64ec for now")
    option(SCXT_USE_FLAC "Include FLAC support" OFF)
    option(SCXT_USE_MP3 "Include MP3 support" OFF)
else()
    option(SCXT_USE_FLAC "Include FLAC support" ON)
    option(SCXT_USE_MP3 "Include MP3 support" ON)
endif()

option(SCXT_SANITIZE "Build with clang/gcc address and undef sanitizer" OFF)
if (WIN32)
    option(SCXT_USE_CLAP_WRAPPER_STANDALONE "Build with the clap wrapper standalone rather than our temp one" ON)
else()
    option(SCXT_USE_CLAP_WRAPPER_STANDALONE "Build with the clap wrapper standalone rather than our temp one" OFF)
endif()

# Share some information about the  build
message(STATUS "Shortcircuit XT ${CMAKE_PROJECT_VERSION}")
message(STATUS "Compiler Version is ${CMAKE_CXX_COMPILER_VERSION}")
if (MSVC)
    message(STATUS "Windows Architecture is ${CMAKE_GENERATOR_PLATFORM}")
endif()


include(cmake/compiler-options.cmake)
include(cmake/CmakeRC.cmake)

# Setup libraries
add_subdirectory(libs)


# Next for now
add_subdirectory(src)
add_subdirectory(resources)
add_subdirectory(src-ui)
add_subdirectory(tests)
add_subdirectory(clients)


include(cmake/basic-installer.cmake)

