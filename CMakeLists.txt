cmake_minimum_required(VERSION 3.10)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9 CACHE STRING "Build for 10.9")

project(ShortCircuit VERSION 3.0.0.0 LANGUAGES CXX ASM)
message( STATUS "CMake Version is ${CMAKE_VERSION}" )
message( STATUS "Compiler Version is ${CMAKE_CXX_COMPILER_VERSION}" )
if( ${CMAKE_SIZEOF_VOID_P} EQUAL 4 )
  message( STATUS "Building in 32 bit configuration" )
else()
  message( STATUS "Building in 64 bit configuration" )
endif()

set(CMAKE_CXX_STANDARD 17)
set(SURGE_CXX_STD_FLAG cxx_std_17)

set( SHORTCIRCUIT_SOURCE
  src/Precompiled.cpp
  src/akai_s6k_import.cpp
  src/battery_kit_import.cpp
  src/biquadunit.cpp
  src/configuration.cpp
  src/configuration_dialog.cpp
  src/cpuarch.cpp
  src/dls_import.cpp
  src/envelope.cpp
  src/filter.cpp
  src/filters_chorus.cpp
  src/filters_delay_based.cpp
  src/filters_destruction.cpp
  src/filters_dualdelay.cpp
  src/filters_dynamics.cpp
  src/filters_eq.cpp
  src/filters_luxury.cpp
  src/filters_modulation.cpp
  src/filters_oscillator.cpp
  src/filters_reverb.cpp
  src/filters_supersvf.cpp
  src/filters_traditional.cpp
  src/filters_v1effects.cpp
  src/generator.cpp
  src/load_aiff.cpp
  src/load_recycle.cpp
  src/load_riff_wave.cpp
  src/load_sf2_sample.cpp
  src/logfile.cpp
  src/mdump.cpp
  src/miniedit_dialog.cpp
  src/modmatrix.cpp
  src/morphEQ.cpp
  src/multiselect.cpp
  src/sample.cpp
  src/sampler.cpp
  src/sampler_automation.cpp
  src/sampler_fileio.cpp
  src/sampler_fileio_riff.cpp
  src/sampler_group.cpp
  src/sampler_gui_interaction.cpp
  src/sampler_notelogic.cpp
  src/sampler_process.cpp
  src/sampler_voice.cpp
  src/scpb_editor.cpp
  src/scpb_sampler.cpp
  src/scpb_vsti.cpp
  src/sf2_import.cpp
  src/sfz_import.cpp
  src/shortcircuit2_RIFF_conversion.cpp
  src/shortcircuit_editor2.cpp
  src/shortcircuit_vsti.cpp
  src/shortcircuit_vsti_edit.cpp
  src/shortcircuit_vsti_main_edit.cpp
  src/steplfo.cpp
  src/threadsafety.cpp
  src/tools.cpp
  src/unitconversion.cpp
  )

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
  libs/vembertech/vt_gui/vt_gui_controls_lfo.cpp
  libs/vembertech/vt_gui/readpng.cpp
  libs/vembertech/vt_gui/vt_gui_window.cpp
  libs/vembertech/vt_gui/scparamedit_layer.cpp
  libs/vembertech/vt_gui/vt_gui_waveedit.cpp
  libs/vembertech/vt_gui/vt_gui_controls_browser.cpp
  libs/vembertech/vt_gui/vt_gui_controls_list.cpp
  libs/vembertech/vt_gui/vt_gui.cpp
  libs/vembertech/vt_gui/vt_gui_controls_menu.cpp
  libs/vembertech/vt_gui/vt_gui_controls.cpp
  libs/vembertech/vt_gui/browserdata.cpp
  libs/vembertech/thread/threadsafety.cpp
)

add_library( shortcircuit-lib
  SHARED
  ${SHORTCIRCUIT_SOURCE}
  ${VEMBERTECH_SOURCE}
)

target_include_directories( shortcircuit-lib
  PRIVATE
  src/
  libs/vembertech
  libs/vembertech/thread
  libs/vembertech/tinyxml
  libs/vembertech/vt_dsp
  libs/vembertech/vt_gui
  libs/vembertech/vt_util
  )

if( APPLE )
  target_compile_definitions( shortcircuit-lib PRIVATE
    MAC=1
    )
elseif( UNIX )
  target_compile_definitions( shortcircuit-lib PRIVATE
    LINUX=1
    )
else()
  target_compile_definitions( shortcircuit-lib PRIVATE
    WINDOWS=1
    )
endif()
