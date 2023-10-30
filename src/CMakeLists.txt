project(scxt-core LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
        browser/browser.cpp
        browser/browser_db.cpp

        datamodel/adsr_storage.cpp

        dsp/generator.cpp
        dsp/data_tables.cpp
        dsp/processor/processor.cpp

        dsp/processor/filter/supersvf.cpp
        dsp/processor/oscillator/oscpulsesync.cpp
        dsp/processor/oscillator/oscsin.cpp
        dsp/processor/oscillator/phasemodulation.cpp

        engine/engine.cpp
        engine/engine_voice_responder.cpp
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

        modulation/base_matrix.cpp
        modulation/group_matrix.cpp
        modulation/voice_matrix.cpp
        modulation/modulators/steplfo.cpp

        selection/selection_manager.cpp

        vembertech/vt_dsp/lipol.cpp

        voice/voice.cpp

        utils.cpp
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

        sst-cpputils
        sst-filters
        sst-basic-blocks
        sst-effects
        sst-plugininfra
        sst-plugininfra::tinyxml
        sst-voicemanager

        libgig libakai
        mts-esp-client
        shortcircuit::simde
        shortcircuit::sqlite3
        taocpp::json

        FLAC++
        sc-compiler-options
        )
