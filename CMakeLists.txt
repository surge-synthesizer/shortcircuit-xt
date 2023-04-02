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
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "Minimum macOS version")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ShortcircuitXT VERSION 0.5.1.0 LANGUAGES C CXX ASM)

# Build Time Options
option(SCXT_BUILD_VST3 "Build a VST3 Plugin" ON)
option(SCXT_BUILD_AU "Build an AudioUnit Plugin" ON)
option(SCXT_BUILD_CLAP "Build a CLAP Plugin" ON)
option(SCXT_BUILD_STANDALONE "Build a Standalone Executable" ON)
option(SCXT_USE_ALSA "Build with ALSA when appropriate" ON)
option(SCXT_USE_JACK "Build with JACK when appropriate" ON)
option(SCXT_COPY_PLUGIN_AFTER_BUILD "Copy plugin after build if possible" ON)

option(SCXT_USE_FLAC "Include flac support from xiph/flac" ON)

option(SCXT_SANITIZE "Build with CLang/gcc address and undef sanitizer" OFF)


# Calculate bitness
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
if (NOT ${BITS} EQUAL 64)
    message(WARNING "${PROJECT_NAME} has only been tested on 64 bits. This may not work")
endif ()

# Share some information about the  build
message(STATUS "ShortCircuit ${CMAKE_PROJECT_VERSION}")
message(STATUS "Compiler Version is ${CMAKE_CXX_COMPILER_VERSION}")

# Everything here is C++ 17 now
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND UNIX AND NOT APPLE AND NOT SURGE_SKIP_PIE_CHANGE)
    message(STATUS "Setting -no-pie on EXE flags; use SURGE_SKIP_PIE_CHANGE=TRUE to avoid")
    set(CMAKE_EXE_LINKER_FLAGS "-no-pie")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    if (${SCXT_SANITIZE})
        message(STATUS "Sanitizer is On")
    endif ()

    # Any Clang or any GCC
    add_compile_options(
            -Wno-multichar
            # Targeting Windows with GCC/Clang is experimental
            # $<$<NOT:$<BOOL:${WIN32}>>:-Werror>

            # PE/COFF doesn't support visibility
            $<$<NOT:$<BOOL:${WIN32}>>:-fvisibility=hidden>

            # Inlines visibility is only relevant with C++
            $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<COMPILE_LANGUAGE:CXX>>:-fvisibility-inlines-hidden>

            # BP note: If you want to turn on llvm/gcc sanitize undo this and the link options below
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=address>
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=undefined>
    )

    add_link_options(
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=address>
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=undefined>
    )

    if (NOT APPLE)
        # Turn on SSE 4.2. Apple has this by default
        add_compile_options(-march=nehalem)
    endif ()
    if (WIN32)
        # Win32 clang marks strncpy as deprecatred and wants strncpy_s; maybe one day
        # fix that but not today
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    endif ()
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # Turn on SSE 4.2
    add_compile_options(/arch:SSE42)
endif ()


include(cmake/compiler-and-git.cmake)

add_subdirectory(libs)

# Next for now
add_subdirectory(src)
add_subdirectory(src-ui)
add_subdirectory(tests)
add_subdirectory(clients)

include(cmake/basic-installer.cmake)


add_custom_target(scxt-code-checks)

# Clang Format checks
find_program(CLANG_FORMAT_EXE NAMES clang-format-12 clang-format)
set(CLANG_FORMAT_DIRS src src-ui tests clients)
set(CLANG_FORMAT_EXTS cpp h)
foreach (dir ${CLANG_FORMAT_DIRS})
    foreach (ext ${CLANG_FORMAT_EXTS})
        list(APPEND CLANG_FORMAT_GLOBS "':(glob)${dir}/**/*.${ext}'")
    endforeach ()
endforeach ()
add_custom_command(TARGET scxt-code-checks
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo About to check clang-format using ${CLANG_FORMAT_EXE}
        COMMAND git ls-files -- ${CLANG_FORMAT_GLOBS} | xargs ${CLANG_FORMAT_EXE} --dry-run --Werror
        )
# }}}