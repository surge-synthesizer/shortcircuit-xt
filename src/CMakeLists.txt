project(scxt-core LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
        browser/browser.cpp

        datamodel/adsr_storage.cpp

        dsp/generator.cpp
        dsp/data_tables.cpp
        dsp/processor/processor.cpp

        dsp/processor/distortion/bitcrusher.cpp
        dsp/processor/distortion/microgate.cpp
        dsp/processor/filter/supersvf.cpp
        dsp/processor/oscillator/oscpulsesync.cpp
        dsp/processor/oscillator/oscsin.cpp
        dsp/processor/oscillator/phasemodulation.cpp

        engine/engine.cpp
        engine/zone.cpp
        engine/group.cpp
        engine/part.cpp
        engine/patch.cpp
        engine/memory_pool.cpp
        engine/bus.cpp

        json/stream.cpp

        sample/sample.cpp
        sample/sample_manager.cpp
        sample/loaders/load_riff_wave.cpp
        sample/loaders/load_aiff.cpp
        sample/loaders/load_flac.cpp

        sfz_support/sfz_parse.cpp
        sfz_support/sfz_import.cpp

        tuning/equal.cpp
        tuning/midikey_retuner.cpp

        infrastructure/file_map_view.cpp

        messaging/audio/audio_messages.cpp
        messaging/messaging.cpp

        modulation/voice_matrix.cpp
        modulation/modulators/steplfo.cpp

        selection/selection_manager.cpp

        vembertech/vt_dsp/lipol.cpp

        voice/voice.cpp

        utils.cpp

        ${CMAKE_BINARY_DIR}/geninclude/version.cpp
        )


if (APPLE)
    target_sources(${PROJECT_NAME} PRIVATE browser/browser_macos.mm)
elseif (WIN32)
    target_sources(${PROJECT_NAME} PRIVATE browser/browser_win.cpp)
else ()
    target_sources(${PROJECT_NAME} PRIVATE browser/browser_lin.cpp)
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC .)
target_link_libraries(${PROJECT_NAME} PUBLIC
        fmt
        sst-cpputils sst-filters sst-basic-blocks sst-effects sst-plugininfra sst-plugininfra::tinyxml
        libgig libakai
        mts-esp-client
        shortcircuit::simde
        taocpp::json

        FLAC++
        sc-compiler-options
        )

target_compile_definitions(${PROJECT_NAME} PUBLIC $<IF:$<CONFIG:DEBUG>,BUILD_IS_DEBUG,BUILD_IS_RELEASE>=1)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _USE_MATH_DEFINES WINDOWS=1)
else ()
    target_compile_options(${PROJECT_NAME} PUBLIC -Werror)
    if (APPLE)
        target_compile_definitions(${PROJECT_NAME} PUBLIC MAC=1)
    else ()
        target_compile_definitions(${PROJECT_NAME} PUBLIC LINUX=1)
    endif ()
endif ()
