# Now set up the sources for the core library

set(SHORTCIRCUIT_SOURCE
        loaders/akai_s6k_import.cpp
        loaders/battery_kit_import.cpp
        synthesis/biquadunit.cpp
        configuration.cpp
        loaders/dls_import.cpp
        synthesis/envelope.cpp
        synthesis/filter.cpp
        synthesis/filters_chorus.cpp
        synthesis/filters_delay_based.cpp
        synthesis/filters_destruction.cpp
        synthesis/filters_dualdelay.cpp
        synthesis/filters_dynamics.cpp
        synthesis/filters_eq.cpp
        synthesis/filters_luxury.cpp
        synthesis/filters_modulation.cpp
        synthesis/filters_oscillator.cpp
        synthesis/filters_reverb.cpp
        synthesis/filters_supersvf.cpp
        synthesis/filters_traditional.cpp
        synthesis/filters_v1effects.cpp
        generator.cpp
        loaders/load_aiff.cpp
        loaders/load_riff_wave.cpp
        loaders/load_sf2_sample.cpp
        infrastructure/logfile.cpp
        infrastructure/ticks.h
        infrastructure/ticks.cpp
        infrastructure/profiler.h
        infrastructure/profiler.cpp
        synthesis/modmatrix.cpp
        synthesis/morphEQ.cpp
        multiselect.cpp
        sample.cpp
        sampler.cpp
        sampler_automation.cpp
        sampler_wrapper_interaction.cpp
        loaders/sampler_fileio.cpp
        loaders/sampler_fileio_riff.cpp
        sampler_notelogic.cpp
        sampler_process.cpp
        sampler_voice.cpp
        loaders/sf2_import.cpp
        loaders/sfz_import.cpp
        loaders/shortcircuit2_RIFF_conversion.cpp
        synthesis/steplfo.cpp
        util/tools.cpp
        util/unitconversion.cpp
        infrastructure/sc3_mmio.cpp infrastructure/sc3_mmio.h infrastructure/file_map_view.cpp infrastructure/file_map_view.h sampler_internalStateView.cpp)

set(SHORTCIRCUIT_GENERATED_SOURCE ${CMAKE_BINARY_DIR}/geninclude/version.cpp)

set(VEMBERTECH_SOURCE
        vembertech/vt_util/vt_string.cpp
        vembertech/vt_util/vt_lockfree.cpp
        vembertech/vt_dsp/halfratefilter.cpp
        vembertech/vt_dsp/lipol.cpp
        vembertech/vt_dsp/lattice.cpp
        vembertech/vt_dsp/macspecific.cpp
        vembertech/vt_dsp/basic_dsp.cpp)


add_library(shortcircuit-core
        ${SHORTCIRCUIT_SOURCE}
        ${VEMBERTECH_SOURCE}
        ${SHORTCIRCUIT_GENERATED_SOURCE})

target_link_libraries(shortcircuit-core PUBLIC
        sst-cpputils
        sst-plugininfra::tinyxml
        fmt::fmt
        sst-plugininfra::filesystem
        shortcircuit::simde
        shortcircuit::compiler-options)
target_include_directories(shortcircuit-core PUBLIC vembertech .)