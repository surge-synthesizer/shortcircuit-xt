# CMakeList.txt to build ShortCircuit3
#
# SC3 began with a not-building snap of the code which was used at some point in the SC2
# product cycle, so we took a different strategy which was
#
# 1. Have a libshortcircuit-core basically which contains all the code to be SC
# 2. Expose that in an shortcircuit-headless, shortcircuitpy library and exe
# 3. Rebuild the plugin using that library and JUCE
#
# At least that's the plan. Critically this no longer builds any of the old GUI or
# VST2 code, which we couldn't make work anyway. That code is still in the codebase for
# reference as we port, but we should remove it before we ship 3.0.0.0
#

cmake_minimum_required(VERSION 3.10)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12 CACHE STRING "Build for 10.12")
project(ShortCircuit VERSION 3.0.0.0 LANGUAGES C CXX ASM)

# Share some information about the  build
message(STATUS "CMake Version is ${CMAKE_VERSION}")
message(STATUS "Compiler Version is ${CMAKE_CXX_COMPILER_VERSION}")
if (${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    message(STATUS "Building in 32 bit configuration")
else ()
    message(STATUS "Building in 64 bit configuration")
endif ()

# Everything here is C++ 17 now
set(CMAKE_CXX_STANDARD 17)

# Set up version information using the same approach as surge, namely
# with an external cmake run, a git-info target, and a generated
# ${bld}/geninclude/version.cpp
execute_process(
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND}
        -D PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        -D PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        -D SHORTCSRC=${CMAKE_SOURCE_DIR}
        -D SHORTCBLD=${CMAKE_BINARY_DIR}
        -D AZURE_PIPELINE=${AZURE_PIPELINE}
        -D WIN32=${WIN32}
        -P ${CMAKE_SOURCE_DIR}/cmake/versiontools.cmake
)
add_custom_target(git-info BYPRODUCTS ${CMAKE_BINARY_DIR}/geninclude/version.cpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND}
        -D PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        -D PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        -D SHORTCSRC=${CMAKE_SOURCE_DIR}
        -D SHORTCBLD=${CMAKE_BINARY_DIR}
        -D AZURE_PIPELINE=${AZURE_PIPELINE}
        -D WIN32=${WIN32}
        -P ${CMAKE_SOURCE_DIR}/cmake/versiontools.cmake
        )

# Include the surge filesystem API so we can work on systems which don't have
# a full std::filesystem (mac targets <10.15, older ubuntus)
add_subdirectory(libs/filesystem)
if( APPLE )
    # Hmmm...
    target_compile_options(filesystem PUBLIC -fvisibility=hidden -fvisibility-inlines-hidden)
elseif( UNIX AND NOT APPLE )
    get_target_property( FSTYPE filesystem TYPE )
    message( STATUS "Linux filesystem is an ${FSTYPE}" )
    if( ${FSTYPE} STREQUAL "INTERFACE_LIBRARY") 
      message( STATUS "  An interface; so no fPIC needed" )
    else()
      message( STATUS "  Adding fPIC and so on to filesystem" )
      target_compile_options(filesystem PUBLIC -fPIC -fvisibility=hidden -fvisibility-inlines-hidden)
    endif()
endif()

# And include JUCE
add_subdirectory(libs/JUCE)

# Platform Specific Compile Settings
if (APPLE)
    set(OS_COMPILE_OPTIONS
            -Wno-invalid-source-encoding
            -Wno-undefined-bool-conversion
            -Wno-format-security
            -Wno-writable-strings

            -fvisibility=hidden -fvisibility-inlines-hidden
            )
    set(OS_COMPILE_DEFINITIONS
            MAC=1)

    set(OS_LINK_LIBRARIES
            "-framework CoreServices"
            )
elseif (UNIX AND NOT APPLE)
    set(OS_COMPILE_OPTIONS
            -Wno-multichar
            -fPIC
            -fvisibility=hidden -fvisibility-inlines-hidden
            )
    set(OS_COMPILE_DEFINITIONS
            LINUX=1)

    set(OS_LINK_LIBRARIES
            )
else ()
    set(OS_COMPILE_OPTIONS
            /wd4244   # convert float from double
            /wd4305   # truncate from double to float
            /wd4018   # signed unsigned compare
            /wd4996   # don't warn on stricmp vs _stricmp and other posix names
            /wd4267   # converting size_t <-> int
            )
    set(OS_COMPILE_DEFINITIONS
            WIN=1
            WINDOWS=1
            _CRT_SECURE_NO_WARNINGS=1
            )

    set(OS_LINK_LIBRARIES
            shell32
            user32

            Winmm
            gdi32
            gdiplus
            msimg32

            ComDlg32
            ComCtl32
            )
endif ()

# Now set up the sources for the core library
set(SHORTCIRCUIT_SOURCE
        src/loaders/akai_s6k_import.cpp
        src/loaders/battery_kit_import.cpp
        src/synthesis/biquadunit.cpp
        src/configuration.cpp
        src/loaders/dls_import.cpp
        src/synthesis/envelope.cpp
        src/synthesis/filter.cpp
        src/synthesis/filters_chorus.cpp
        src/synthesis/filters_delay_based.cpp
        src/synthesis/filters_destruction.cpp
        src/synthesis/filters_dualdelay.cpp
        src/synthesis/filters_dynamics.cpp
        src/synthesis/filters_eq.cpp
        src/synthesis/filters_luxury.cpp
        src/synthesis/filters_modulation.cpp
        src/synthesis/filters_oscillator.cpp
        src/synthesis/filters_reverb.cpp
        src/synthesis/filters_supersvf.cpp
        src/synthesis/filters_traditional.cpp
        src/synthesis/filters_v1effects.cpp
        src/generator.cpp
        src/loaders/load_aiff.cpp
        src/loaders/load_riff_wave.cpp
        src/loaders/load_sf2_sample.cpp
        src/infrastructure/logfile.cpp
        src/synthesis/modmatrix.cpp
        src/synthesis/morphEQ.cpp
        src/multiselect.cpp
        src/sample.cpp
        src/sampler.cpp
        src/sampler_automation.cpp
        src/loaders/sampler_fileio.cpp
        src/loaders/sampler_fileio_riff.cpp
        src/sampler_notelogic.cpp
        src/sampler_process.cpp
        src/sampler_voice.cpp
        src/loaders/sf2_import.cpp
        src/loaders/sfz_import.cpp
        src/loaders/shortcircuit2_RIFF_conversion.cpp
        src/synthesis/steplfo.cpp
        src/util/tools.cpp
        src/util/unitconversion.cpp
        src/infrastructure/sc3_mmio.cpp src/infrastructure/sc3_mmio.h src/infrastructure/file_map_view.cpp src/infrastructure/file_map_view.h)

set(SHORTCIRCUIT_GENERATED_SOURCE
        ${CMAKE_BINARY_DIR}/geninclude/version.cpp)

set(VEMBERTECH_SOURCE
        src/vembertech/tinyxml/tinyxmlparser.cpp
        src/vembertech/tinyxml/tinyxmlerror.cpp
        src/vembertech/tinyxml/tinyxml.cpp
        src/vembertech/vt_util/vt_string.cpp
        src/vembertech/vt_util/vt_lockfree.cpp
        src/vembertech/vt_dsp/halfratefilter.cpp
        src/vembertech/vt_dsp/lipol.cpp
        src/vembertech/vt_dsp/lattice.cpp
        src/vembertech/vt_dsp/macspecific.cpp
        src/vembertech/vt_dsp/basic_dsp.cpp
        )

set(SHORTCIRCUIT_INCLUDE_DIRS
        src/
        src/vembertech  # used to be in libs but don't rewrite every ref
        )

add_library(shortcircuit-core
        ${SHORTCIRCUIT_SOURCE}
        ${VEMBERTECH_SOURCE}
        ${SHORTCIRCUIT_GENERATED_SOURCE})

# The core library includes most of the exports and flags you need
target_include_directories(shortcircuit-core PUBLIC ${SHORTCIRCUIT_INCLUDE_DIRS})
target_compile_options(shortcircuit-core PUBLIC ${OS_COMPILE_OPTIONS})
target_compile_definitions(shortcircuit-core PUBLIC ${OS_COMPILE_DEFINITIONS} TARGET_HEADLESS=1)
target_link_libraries(shortcircuit-core PUBLIC ${OS_LINK_LIBRARIES} surge::filesystem)
add_dependencies(shortcircuit-core git-info)

# Executable Target sc-headless is primarily for throwaway tests and has been subsumed by
# sc3-test and the python library. Keep it nonetheless
add_executable(sc-headless wrappers/headless/main.cpp)
target_link_libraries(sc-headless PRIVATE shortcircuit-core)

# The JUCE plugin ShortCircuit3 has an AU VST3 and Standalone version
juce_add_plugin(ShortCircuit3
        PRODUCT_NAME "ShortCircuit3"
        COMPANY_NAME "Surge Synth Team"
        BUNDLE_ID "org.surge-synth-team.shortcircuit3"
        PLUGIN_MANUFACTURER_CODE VmbA
        PLUGIN_CODE SCt3

        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE

        FORMATS AU VST3 Standalone
        )

juce_generate_juce_header(ShortCircuit3)

# Add a binary resource (needs remediation)
# file(GLOB SC3_RESOURCES_GLOB resources/test_samples/*)
set( SC3_RESOURCES_GLOB resources/shortcircuit.ico )
juce_add_binary_data(ShortCircuit3-Binary SOURCES ${SC3_RESOURCES_GLOB})
set_target_properties(ShortCircuit3-Binary PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# Then set up the source and the JUCE settings
target_sources(ShortCircuit3 PRIVATE
        wrappers/juce/SC3Processor.cpp
        wrappers/juce/SC3Editor.cpp
        )
target_compile_definitions(ShortCircuit3 PUBLIC
        JUCE_ALLOW_STATIC_NULL_VARIABLES=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1

        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0

        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0

        JUCE_ALSA=1
        JUCE_JACK=1
        )
target_link_libraries(ShortCircuit3 PRIVATE
        ShortCircuit3-Binary
        shortcircuit-core
        juce::juce_analytics
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_blocks_basics
        juce::juce_box2d
        juce::juce_dsp
        )


# Build the python bindings
message(STATUS "Building ShortCircuit Python bindings with pybind11")
add_subdirectory(libs/pybind11)
pybind11_add_module(shortcircuit3py)
target_sources(shortcircuit3py PRIVATE wrappers/py/sc3py.cpp)
target_link_libraries(shortcircuit3py PRIVATE shortcircuit-core)


# And finally the test suite
add_executable(sc3-test
        tests/lowlevel_io.cpp
        tests/mmio_test.cpp
        tests/test_main.cpp
        tests/format_tests.cpp
        tests/vt_miscellany.cpp
        tests/config_test.cpp
        tests/zone_tests.cpp)

target_include_directories(sc3-test PRIVATE
        libs/catch2/include)

target_link_libraries(sc3-test
        shortcircuit-core
        surge::filesystem
        )

# Once we have our assets built, lets move them to our product directory
set(SC3_PRODUCT_DIR ${CMAKE_BINARY_DIR}/products)
file(MAKE_DIRECTORY ${SC3_PRODUCT_DIR})

add_custom_target(sc3-products)
add_dependencies(sc3-products sc3-test)
add_custom_command(
        TARGET sc3-products
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND cmake -E copy $<TARGET_FILE:sc3-test> ${SC3_PRODUCT_DIR}
)
add_dependencies(sc3-products shortcircuit3py)
add_custom_command(
        TARGET sc3-products
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND cmake -E copy $<TARGET_FILE:shortcircuit3py> ${SC3_PRODUCT_DIR}
)

# TODO: Move the JUCE assets into products
add_dependencies(sc3-products ShortCircuit3)
