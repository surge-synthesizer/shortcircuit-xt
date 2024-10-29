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
message(STATUS "ShortcircuitXT: Using date-driven version while in alpha")
string(TIMESTAMP DAY_OF_YEAR "%j")
string(TIMESTAMP YEAR "%Y")
math(EXPR PART0 "${YEAR}-2023 + 6")
math(EXPR PART1 "${DAY_OF_YEAR} + 1")

project(ShortcircuitXT VERSION 0.${PART0}.${PART1}.0 LANGUAGES C CXX ASM)
set(ShortcircuitXT_VERSION ${PROJECT_VERSION})

if (APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
    set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
    if( ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "15.0.0" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "15.1")
        add_link_options(-Wl,-ld_classic)
        add_compile_definitions(JUCE_SILENCE_XCODE_15_LINKER_WARNING=1)
    endif()
endif ()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Never want shared if not specified")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)


# Build Time Options
option(SCXT_BUILD_VST3 "Build a VST3 plugin" ON)
option(SCXT_BUILD_AU "Build an AU plugin" ON)
option(SCXT_BUILD_CLAP "Build a CLAP" ON)
option(SCXT_BUILD_STANDALONE "Build a standalone executable" ON)
option(SCXT_USE_ALSA "Build with ALSA when appropriate" ON)
option(SCXT_USE_JACK "Build with JACK when appropriate" ON)
option(SCXT_COPY_PLUGIN_AFTER_BUILD "Copy plugin after build if possible" OFF)
option(SCXT_SKIP_PIE_CHANGE "Dont add -no-pie to executables" OFF)

option(SCXT_USE_FLAC "Include FLAC support" ON)
option(SCXT_USE_MP3 "Include MP3 support" ON)

option(SCXT_SANITIZE "Build with clang/gcc address and undef sanitizer" OFF)
if (WIN32)
    option(SCXT_USE_CLAP_WRAPPER_STANDALONE "Build with the clap wrapper standalone rather than our temp one" ON)
else()
    option(SCXT_USE_CLAP_WRAPPER_STANDALONE "Build with the clap wrapper standalone rather than our temp one" OFF)
endif()

# Calculate bitness
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
if (NOT ${BITS} EQUAL 64)
    message(WARNING "${PROJECT_NAME} has only been tested on 64 bits. This may not work")
endif ()

# Share some information about the  build
message(STATUS "Shortcircuit XT ${CMAKE_PROJECT_VERSION}")
message(STATUS "Compiler Version is ${CMAKE_CXX_COMPILER_VERSION}")
if (MSVC)
    message(STATUS "Windows Architecture is ${CMAKE_GENERATOR_PLATFORM}}")
endif()

# Everything here is C++ 17 now
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND UNIX AND NOT APPLE AND NOT SCXT_SKIP_PIE_CHANGE)
    message(STATUS "Setting -no-pie on EXE flags; use SCXT_SKIP_PIE_CHANGE=TRUE to avoid")
    set(CMAKE_EXE_LINKER_FLAGS "-no-pie")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    if (${SCXT_SANITIZE})
        message(STATUS "Sanitizer is ON")
    endif ()

    # BP note: If you want to turn on llvm/gcc sanitize, remove this and the link options below
    add_compile_options(
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=address>
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=undefined>
    )

    add_link_options(
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=address>
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=undefined>
    )
endif ()

add_compile_definitions(SCXT_ROOT_BUILD_DIR="${CMAKE_SOURCE_DIR}")

include(cmake/compiler-and-git.cmake)
include(cmake/CmakeRC.cmake)

add_subdirectory(libs)

# Next for now
add_subdirectory(src)
add_subdirectory(resources)
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