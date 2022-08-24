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
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14 CACHE STRING "Minimum macOS version")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(ShortCircuit VERSION 0.5.0.0 LANGUAGES C CXX ASM)

# Calculate bitness
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
message(STATUS "Targeting ${BITS}-bit configuration")

# Share some information about the  build
message(STATUS "ShortCircuit ${CMAKE_PROJECT_VERSION}")
message(STATUS "Compiler Version is ${CMAKE_CXX_COMPILER_VERSION}")


# Optional variables
option(SCXT_SKIP_PYTHON "Skip the Python Wrapper" FALSE)


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
endif ()

include(cmake/compiler-and-git.cmake)

add_subdirectory(libs)
add_subdirectory(src)
add_subdirectory(wrappers)
add_subdirectory(tests)

if (DEFINED ENV{ASIOSDK_DIR} OR BUILD_USING_MY_ASIO_LICENSE)
    if (BUILD_USING_MY_ASIO_LICENSE)
        message(STATUS "** BUILD USING OWN ASIO LICENSE **")
        message(STATUS "The resulting Shortcircuit XT standalone executable is not licensed for distribution!")
        message(STATUS "Fetching ASIO SDK...")

        set(ASIOSDK_DIR ${CMAKE_BINARY_DIR}/asio/asiosdk)
        add_custom_target(scxt-get-local-asio)
        add_custom_command(
                TARGET scxt-get-local-asio
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/asio
                COMMAND ${CMAKE_COMMAND} -D ASIO_SDK_DESTINATION=${CMAKE_BINARY_DIR}/asio -P cmake/get-asio.cmake
        )
        add_dependencies(ShortcircuitXT scxt-get-local-asio)
    else ()
        file(TO_CMAKE_PATH "$ENV{ASIOSDK_DIR}" ASIOSDK_DIR)
        message(STATUS "ASIO SDK found at ${ASIOSDK_DIR}")
        message(STATUS "The resulting Shortcircuit XT standalone executable is not licensed for distribution!")
    endif ()

    target_compile_definitions(ShortcircuitXT PUBLIC JUCE_ASIO=1)
    target_include_directories(ShortcircuitXT PUBLIC ${ASIOSDK_DIR}/common)
    set(JUCE_ASIO_SUPPORT TRUE)
endif ()

include(cmake/code-quality.cmake)
include(cmake/basic-installer.cmake)
