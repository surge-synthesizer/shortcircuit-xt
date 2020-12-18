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
message( STATUS "CMake Version is ${CMAKE_VERSION}" )
message( STATUS "Compiler Version is ${CMAKE_CXX_COMPILER_VERSION}" )
if( ${CMAKE_SIZEOF_VOID_P} EQUAL 4 )
  message( STATUS "Building in 32 bit configuration" )
else()
  message( STATUS "Building in 64 bit configuration" )
endif()

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
add_custom_target( git-info BYPRODUCTS ${CMAKE_BINARY_DIR}/geninclude/version.cpp
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

# And include JUCE
add_subdirectory(libs/JUCE)

# Platform Specific Compile Settings
if( APPLE )
  set( OS_COMPILE_OPTIONS
          -Wno-invalid-source-encoding
          -Wno-undefined-bool-conversion
          -Wno-format-security
          -Wno-writable-strings
          )
  set( OS_COMPILE_DEFINITIONS
          MAC=1 )

  set( OS_LINK_LIBRARIES
          "-framework CoreServices"
          )
elseif( UNIX AND NOT APPLE )
  set( OS_COMPILE_OPTIONS
          )
  set( OS_COMPILE_DEFINITIONS
          LINUX=1 )

  set( OS_LINK_LIBRARIES
          )
else()
  set( OS_COMPILE_OPTIONS )
  set( OS_COMPILE_DEFINITIONS
     WIN=1
     WINDOWS=1
    _CRT_SECURE_NO_WARNINGS=1
          )

  set( OS_LINK_LIBRARIES
          shell32
          user32

          Winmm
          gdi32
          gdiplus
          msimg32

          ComDlg32
          ComCtl32
          )
endif()

# Now set up the sources for the core library
set( SHORTCIRCUIT_SOURCE
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
        src/loaders/load_recycle.cpp
        src/loaders/load_riff_wave.cpp
        src/loaders/load_sf2_sample.cpp
        src/infrastructure/logfile.cpp
  src/mdump.cpp
        src/synthesis/modmatrix.cpp
        src/synthesis/morphEQ.cpp
  src/multiselect.cpp
  src/sample.cpp
  src/sampler.cpp
  src/sampler_automation.cpp
  src/sampler_fileio.cpp
  src/sampler_fileio_riff.cpp
  src/sampler_notelogic.cpp
  src/sampler_process.cpp
  src/sampler_voice.cpp
        src/loaders/sf2_import.cpp
        src/loaders/sfz_import.cpp
        src/loaders/shortcircuit2_RIFF_conversion.cpp
        src/synthesis/steplfo.cpp
  src/tools.cpp
  src/unitconversion.cpp
  )

set( SHORTCIRCUIT_GENERATED_SOURCE
        ${CMAKE_BINARY_DIR}/geninclude/version.cpp )

set( VEMBERTECH_SOURCE
  libs/vembertech/tinyxml/tinystr.cpp
  libs/vembertech/tinyxml/tinyxmlparser.cpp
  libs/vembertech/tinyxml/tinyxmlerror.cpp
  libs/vembertech/tinyxml/tinyxml.cpp
  libs/vembertech/vt_util/vt_string.cpp
  libs/vembertech/vt_util/vt_lockfree.cpp
  libs/vembertech/vt_dsp/halfratefilter.cpp
  libs/vembertech/vt_dsp/halfratefilter_ppc.cpp
  libs/vembertech/vt_dsp/lipol.cpp
  libs/vembertech/vt_dsp/lattice.cpp
  libs/vembertech/vt_dsp/macspecific.cpp
  libs/vembertech/vt_dsp/basic_dsp.cpp
  libs/vembertech/thread/threadsafety.cpp
)

set( SHORTCIRCUIT_INCLUDE_DIRS
     src/
     libs/vembertech
)

add_library(shortcircuit-core
        ${SHORTCIRCUIT_SOURCE}
        ${VEMBERTECH_SOURCE}
        ${SHORTCIRCUIT_GENERATED_SOURCE})

target_include_directories(shortcircuit-core PRIVATE ${SHORTCIRCUIT_INCLUDE_DIRS} )
target_compile_options(shortcircuit-core PRIVATE ${OS_COMPILE_OPTIONS})
target_compile_definitions(shortcircuit-core PRIVATE ${OS_COMPILE_DEFINITIONS} TARGET_HEADLESS=1)
add_dependencies(shortcircuit-core git-info)

add_executable( sc-headless
  wrappers/headless/main.cpp
  )

target_include_directories( sc-headless PRIVATE ${SHORTCIRCUIT_INCLUDE_DIRS} )
target_compile_options(sc-headless PRIVATE ${OS_COMPILE_OPTIONS})
target_compile_definitions(sc-headless PRIVATE ${OS_COMPILE_DEFINITIONS} TARGET_HEADLESS=1)
target_link_libraries(sc-headless PRIVATE
        ${OS_LINK_LIBRARIES}
        shortcircuit-core
        surge::filesystem
        )

juce_add_plugin( ShortCircuit3
  PRODUCT_NAME "ShortCircuit3"
  COMPANY_NAME "Surge Synth Team"
  BUNDLE_ID "org.surge-synth-team.shortcircuit3"
  PLUGIN_MANUFACTURER_CODE VmbA
  PLUGIN_CODE SCt3

  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE

  FORMATS AU VST3 Standalone
  )

juce_generate_juce_header( ShortCircuit3 )

# Fix me this dies obviously
file( GLOB SC3_RESOURCES_GLOB resources/test_samples/* )
juce_add_binary_data( ShortCircuit3-Binary SOURCES ${SC3_RESOURCES_GLOB} )
set_target_properties( ShortCircuit3-Binary PROPERTIES POSITION_INDEPENDENT_CODE TRUE )

target_include_directories( ShortCircuit3
  PRIVATE
  ${SHORTCIRCUIT_INCLUDE_DIRS}
  )

target_sources( ShortCircuit3 PRIVATE
  wrappers/juce/SC3Processor.cpp
  wrappers/juce/SC3Editor.cpp
  )
target_compile_options(ShortCircuit3 PUBLIC ${OS_COMPILE_OPTIONS})
target_compile_definitions( ShortCircuit3 PUBLIC
        ${OS_COMPILE_DEFINITIONS}
        TARGET_HEADLESS=1
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

target_link_libraries( ShortCircuit3 PRIVATE
  ShortCircuit3-Binary
  shortcircuit-core
  juce::juce_analytics
  juce::juce_audio_utils
  juce::juce_audio_processors
  juce::juce_blocks_basics
  juce::juce_box2d
  juce::juce_dsp
)


if( NOT WIN32 )
  message( STATUS "Building ShortCircuit Python bindings with pybind11" )
  add_subdirectory(libs/pybind11)

  pybind11_add_module( shortcircuit3py )
  target_sources( shortcircuit3py PRIVATE wrappers/py/sc3py.cpp )
  target_include_directories(shortcircuit3py PUBLIC ${SHORTCIRCUIT_INCLUDE_DIRS})
  target_compile_options(shortcircuit3py PUBLIC ${OS_COMPILE_OPTIONS})
  target_compile_definitions( shortcircuit3py PUBLIC
          ${OS_COMPILE_DEFINITIONS}
          TARGET_HEADLESS=1
          )


  target_link_libraries( shortcircuit3py PRIVATE
          shortcircuit-core
          surge::filesystem
          )
endif()
