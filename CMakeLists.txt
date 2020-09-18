#
# This is all a mess as I hack away right now
#

cmake_minimum_required(VERSION 3.10)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9 CACHE STRING "Build for 10.9")

project(ShortCircuit VERSION 3.0.0.0 LANGUAGES C CXX ASM)
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
  src/modmatrix.cpp
  src/morphEQ.cpp
  src/multiselect.cpp
  src/sample.cpp
  src/sampler.cpp
  src/sampler_automation.cpp
  src/sampler_fileio.cpp
  src/sampler_fileio_riff.cpp
  src/sampler_notelogic.cpp
  src/sampler_process.cpp
  src/sampler_voice.cpp
  src/sf2_import.cpp
  src/sfz_import.cpp
  src/shortcircuit2_RIFF_conversion.cpp
  src/steplfo.cpp
  src/tools.cpp
  src/unitconversion.cpp

  )

set( SHORTCIRCUIT_GUI_SOURCE
  src/sampler_gui_interaction.cpp
  # Windows only later on
  src/vtgiurc.rc
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
  libs/vembertech/thread/threadsafety.cpp
)

set( VEMBERTECH_GUI_SOURCE
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
)

set( VST2_WRAPPER_SOURCE
  wrappers/vst2/shortcircuit_editor2.cpp
  wrappers/vst2/shortcircuit_vsti.cpp
  wrappers/vst2/shortcircuit_vsti_edit.cpp
  wrappers/vst2/shortcircuit_vsti_main_edit.cpp
)



if( DEFINED ENV{VST2SDK_DIR} )
  file( TO_CMAKE_PATH "$ENV{VST2SDK_DIR}" VSTHACK )
else()
  set( VSTHACK "${CMAKE_SOURCE_DIR}/../../VST2_SDK/" )
endif()

set( VST2_LIB_SOURCE
  ${VSTHACK}/public.sdk/source/vst2.x/audioeffect.cpp
  ${VSTHACK}/public.sdk/source/vst2.x/audioeffectx.cpp
  )

set( ZLIB_SOURCE
  libs/external/zlib/adler32.c
  libs/external/zlib/compress.c
  libs/external/zlib/crc32.c
  libs/external/zlib/deflate.c
  libs/external/zlib/example.c
  libs/external/zlib/gzio.c
  libs/external/zlib/infback.c
  libs/external/zlib/inffast.c
  libs/external/zlib/inflate.c
  libs/external/zlib/inftrees.c
  libs/external/zlib/trees.c
  libs/external/zlib/uncompr.c
  libs/external/zlib/zutil.c
)

add_library( shortcircuit-lib
  SHARED
  ${SHORTCIRCUIT_SOURCE}
  ${SHORTCIRCUIT_GUI_SOURCE}
  ${VEMBERTECH_SOURCE}
  ${VEMBERTECH_GUI_SOURCE}
  ${VST2_WRAPPER_SOURCE}
  ${VST2_LIB_SOURCE}
  ${ZLIB_SOURCE}
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

  # Temporary until I refactor the VST2
  wrappers/vst2
  ${VSTHACK}
  ${VSTHACK}/public.sdk/source/vst2.x
  )



if( WIN32 )
  target_include_directories( shortcircuit-lib PRIVATE libs/external/libpng libs/external/zlib )
endif()

if( APPLE )
  target_compile_definitions( shortcircuit-lib PRIVATE
    MAC=1
    )
  target_compile_options( shortcircuit-lib PRIVATE
    -Wno-invalid-source-encoding
    )
elseif( UNIX )
  target_compile_definitions( shortcircuit-lib PRIVATE
    LINUX=1
    )
else()
    if( ${CMAKE_SIZEOF_VOID_P} EQUAL 4 )
    set( PNG_LINK_DIR "libs/external/libpng/" )
  else()
    set( PNG_LINK_DIR "libs/external/libpng/x64" )
  endif()
  message( STATUS "Configuring PNG linkage from ${PNG_LINK_DIR}" )

  target_compile_definitions( shortcircuit-lib PRIVATE
    WINDOWS=1
    TARGET_VST2=1
    
    _CRT_SECURE_NO_WARNINGS=1
    )
  target_link_libraries( shortcircuit-lib
    PRIVATE
    
    shell32
    user32

    Winmm
    gdi32
    gdiplus
    msimg32
    
    ComDlg32
    ComCtl32

    ${CMAKE_SOURCE_DIR}/${PNG_LINK_DIR}/libpng.lib
    ${CMAKE_SOURCE_DIR}/${PNG_LINK_DIR}/zlib.lib
    )

  target_link_options(shortcircuit-lib PUBLIC "/DEF:${CMAKE_SOURCE_DIR}\\resources\\windows\\shortcircuit_vst2.def" )

endif()

add_executable( sc-headless
  ${SHORTCIRCUIT_SOURCE}
  ${VEMBERTECH_SOURCE}
  wrappers/headless/main.cpp
  )

target_include_directories( sc-headless
  PRIVATE
  src/
  libs/vembertech
  libs/vembertech/thread
  libs/vembertech/tinyxml
  libs/vembertech/vt_dsp
  libs/vembertech/vt_util
  )

if( APPLE )
  target_compile_definitions( sc-headless PRIVATE
    MAC=1
    TARGET_HEADLESS=1
    )
  target_compile_options( sc-headless PRIVATE
    -Wno-invalid-source-encoding
    )

endif()

if( WIN32 )
  target_compile_definitions( sc-headless PRIVATE
    WIN=1
    WINDOWS=1
    TARGET_HEADLESS=1
    )

  target_link_libraries( sc-headless
    PRIVATE
    
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
