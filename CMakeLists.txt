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
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

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

# And include JUCE
add_subdirectory(libs/JUCE)

# Optional variables
set(SCXT_INCLUDE_ASIO_SUPPORT FALSE)

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
    if (SCXT_INCLUDE_ASIO_SUPPORT)
        execute_process(COMMAND powershell
                -ExecutionPolicy Bypass
                -File "scripts/win_dl_asiosdk.ps1"
                )
    endif()
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
        src/infrastructure/ticks.h
        src/infrastructure/ticks.cpp
        src/infrastructure/profiler.h
        src/infrastructure/profiler.cpp
        src/synthesis/modmatrix.cpp
        src/synthesis/morphEQ.cpp
        src/multiselect.cpp
        src/sample.cpp
        src/sampler.cpp
        src/sampler_automation.cpp
        src/sampler_wrapper_interaction.cpp
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
        src/infrastructure/sc3_mmio.cpp src/infrastructure/sc3_mmio.h src/infrastructure/file_map_view.cpp src/infrastructure/file_map_view.h src/sampler_internalStateView.cpp src/infrastructure/import_fs.h)

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
        src/vembertech/vt_dsp/basic_dsp.cpp)

set(SHORTCIRCUIT_INCLUDE_DIRS
        src/
        src/vembertech  # used to be in libs but don't rewrite every ref
        )

add_library(shortcircuit-core
        ${SHORTCIRCUIT_SOURCE}
        ${VEMBERTECH_SOURCE}
        ${SHORTCIRCUIT_GENERATED_SOURCE})

# Add a filesystem implementation if we need it
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
    #include <filesystem>
    namespace fs = std::filesystem;
    int main(int, char **argv) {
      return fs::recursive_directory_iterator{fs::path{*argv}}.depth();
    }" SC3_FILESYSTEM_STD_FS_FOUND)
if( SC3_FILESYSTEM_STD_FS_FOUND )
    message(STATUS "Using OS level Filesystem" )
else()
    message(STATUS "Using GHC Filesystem" )
    add_subdirectory(libs/ghc-filesystem)
    target_compile_definitions(shortcircuit-core PUBLIC SC3_USE_GHC_FILESYSTEM=1)
    target_link_libraries(shortcircuit-core PUBLIC ghc_filesystem)

endif()

# Build a target and directory for our final products
set(SC3_PRODUCT_DIR ${CMAKE_BINARY_DIR}/products)
file(MAKE_DIRECTORY ${SC3_PRODUCT_DIR})
add_custom_target(sc3-products)

# The core library includes most of the exports and flags you need
target_include_directories(shortcircuit-core PUBLIC ${SHORTCIRCUIT_INCLUDE_DIRS})
target_compile_options(shortcircuit-core PUBLIC ${OS_COMPILE_OPTIONS})
target_compile_definitions(shortcircuit-core PUBLIC ${OS_COMPILE_DEFINITIONS} TARGET_HEADLESS=1)
target_link_libraries(shortcircuit-core PUBLIC ${OS_LINK_LIBRARIES})
add_dependencies(shortcircuit-core git-info)

# Executable Target sc3-headless is primarily for throwaway tests and has been subsumed by
# sc3-test and the python library. Keep it nonetheless
add_executable(sc3-headless wrappers/headless/main.cpp)
target_link_libraries(sc3-headless PRIVATE shortcircuit-core)

# The JUCE plugin Shortcircuit XT has an AU, VST3 and standalone version
juce_add_plugin(ShortcircuitXT
        PRODUCT_NAME "Shortcircuit XT"
        COMPANY_NAME "Surge Synth Team"
        BUNDLE_ID "org.surge-synth-team.shortcircuit-xt"
        PLUGIN_MANUFACTURER_CODE VmbA
        PLUGIN_CODE ScXT

        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE

        FORMATS AU VST3 Standalone
        )

juce_generate_juce_header(ShortcircuitXT)

# Add a binary resource (needs remediation)
# file(GLOB SC3_RESOURCES_GLOB resources/test_samples/*)
set( SC3_RESOURCES_GLOB resources/shortcircuit.ico )
juce_add_binary_data(ShortcircuitXT-Binary SOURCES ${SC3_RESOURCES_GLOB})
set_target_properties(ShortcircuitXT-Binary PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# images used in the ui
juce_add_binary_data(ImageAssets NAMESPACE SCXTImages SOURCES
        resources/images/wavehandle_start.png
        resources/images/wavehandle_end.png
        resources/images/wavehandle_crossfade.png
        )

set_target_properties(ImageAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)


# Then set up the source and the JUCE settings
target_sources(ShortcircuitXT PRIVATE
        wrappers/juce/SC3Processor.cpp
        wrappers/juce/SC3Editor.cpp

        wrappers/juce/components/WaveDisplay.cpp
        wrappers/juce/components/WaveDisplay.h
        wrappers/juce/components/DebugPanel.cpp
        wrappers/juce/components/DebugPanel.h
        wrappers/juce/components/StubRegion.h
        wrappers/juce/components/ZoneKeyboardDisplay.cpp
        wrappers/juce/components/ZoneKeyboardDisplay.h

        wrappers/juce/proxies/ZoneStateProxy.cpp
        wrappers/juce/proxies/ZoneStateProxy.h

        wrappers/juce/scratchpad.cpp
        wrappers/juce/scratchpad.h
        )
target_include_directories(ShortcircuitXT PRIVATE wrappers/juce )
target_compile_definitions(ShortcircuitXT PUBLIC
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
# ASIO SDK is only for MS Windows.
if (NOT APPLE AND NOT UNIX AND SCXT_INCLUDE_ASIO_SUPPORT)
    target_compile_definitions(ShortcircuitXT PUBLIC
        JUCE_ASIO=1
        )
    target_include_directories(ShortcircuitXT PUBLIC
       "libs/asiosdk/common"
       )
endif()
target_link_libraries(ShortcircuitXT PRIVATE
        ImageAssets
        ShortcircuitXT-Binary
        shortcircuit-core
        juce::juce_analytics
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_blocks_basics
        juce::juce_box2d
        juce::juce_dsp
        )


# Build the python bindings
if(SHORTCIRCUIT_XT_SKIP_PYTHON)
    message( STATUS "Skipping Python Build" )
else()
    message(STATUS "Building ShortCircuit Python bindings with pybind11")
    add_subdirectory(libs/pybind11)
    pybind11_add_module(shortcircuit3py)
    target_sources(shortcircuit3py PRIVATE wrappers/py/sc3py.cpp)
    target_link_libraries(shortcircuit3py PRIVATE shortcircuit-core)

    add_dependencies(sc3-products shortcircuit3py)
    add_custom_command(
            TARGET sc3-products
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND cmake -E copy $<TARGET_FILE:shortcircuit3py> ${SC3_PRODUCT_DIR}
     )
endif()

# And finally the test suite
add_executable(sc3-test
        tests/lowlevel_io.cpp
        tests/mmio_test.cpp
        tests/test_main.cpp
        tests/format_tests.cpp
        tests/vt_miscellany.cpp
        tests/config_test.cpp
        tests/logging_test.cpp
        tests/profiler_test.cpp
        tests/zone_tests.cpp tests/filesystem_basics.cpp)

target_include_directories(sc3-test PRIVATE
        libs/catch2/include)

target_link_libraries(sc3-test
        shortcircuit-core
        )

add_dependencies(sc3-products sc3-test)

add_custom_command(
        TARGET sc3-products
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND cmake -E copy $<TARGET_FILE:sc3-test> ${SC3_PRODUCT_DIR}
        COMMAND cmake -E copy ${CMAKE_SOURCE_DIR}/LICENSE ${SC3_PRODUCT_DIR}
        COMMAND cmake -E copy ${CMAKE_SOURCE_DIR}/resources/installer_README.txt ${SC3_PRODUCT_DIR}/README.txt
)

# TODO: Move the JUCE assets into products
add_dependencies(sc3-products ShortcircuitXT)

add_dependencies(sc3-products ShortcircuitXT_VST3)
add_dependencies(sc3-products ShortcircuitXT_Standalone)
get_target_property(JUCE_DIR ShortcircuitXT LIBRARY_OUTPUT_DIRECTORY)
add_custom_command(
        TARGET sc3-products
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND cmake -E copy_directory ${JUCE_DIR}/VST3/ ${SC3_PRODUCT_DIR}
        COMMAND cmake -E copy_directory ${JUCE_DIR}/Standalone/ ${SC3_PRODUCT_DIR}
)

if( APPLE )
    add_dependencies(sc3-products ShortcircuitXT_AU)
    add_custom_command(
            TARGET sc3-products
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND cmake -E copy_directory ${JUCE_DIR}/AU/ ${SC3_PRODUCT_DIR}
    )
endif()

add_custom_target(sc3-installer)
add_dependencies(sc3-installer sc3-products)
file(READ ${CMAKE_BINARY_DIR}/geninclude/githash.txt GIT_COMMIT_HASH)
if( APPLE )
    add_custom_command(
            TARGET sc3-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND cmake -E make_directory installer/macos
            COMMAND hdiutil create installer/macos/ShortcircuitXT-macos-${GIT_COMMIT_HASH}.dmg -ov -volname "ShortcircuitXT ${GIT_COMMIT_HASH}" -fs HFS+ -srcfolder products
    )
elseif( WIN32 )
    file( TO_NATIVE_PATH "installer/windows/ShortcircuitXT-win64-${GIT_COMMIT_HASH}.zip" OUTPUT_ZIP )
    add_custom_command(
            TARGET sc3-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory  installer/windows
            COMMAND powershell Compress-Archive
                -DestinationPath "${OUTPUT_ZIP}"
                -Force
                -Path "products"
    )

else()
    add_custom_command(
            TARGET sc3-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND cmake -E make_directory installer/linux
            COMMAND tar czf installer/linux/ShortcircuitXT-linux-${GIT_COMMIT_HASH}.tgz products
    )
endif()
