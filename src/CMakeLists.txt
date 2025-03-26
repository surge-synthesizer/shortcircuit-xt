project(scxt-core LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
        browser/browser.cpp
        browser/browser_db.cpp
        browser/scanner.cpp

        dsp/generator.cpp
        dsp/data_tables.cpp
        dsp/processor/processor.cpp
        dsp/sample_analytics.cpp

        engine/engine.cpp
        engine/engine_voice_responder.cpp
        engine/zone.cpp
        engine/group.cpp
        engine/part.cpp
        engine/patch.cpp
        engine/memory_pool.cpp
        engine/missing_resolution.cpp
        engine/bus.cpp
        engine/macros.cpp
        engine/group_triggers.cpp

        json/stream.cpp

        sample/sample.cpp
        sample/sample_manager.cpp
        sample/loaders/load_riff_wave.cpp
        sample/loaders/load_aiff.cpp
        sample/loaders/load_flac.cpp
        sample/loaders/load_mp3.cpp

        sample/exs_support/exs_import.cpp
        sample/multisample_support/multisample_import.cpp
        sample/sf2_support/sf2_compound_file_impl.cpp
        sample/sf2_support/sf2_import.cpp
        sample/sfz_support/sfz_compound_file_impl.cpp
        sample/sfz_support/sfz_parse.cpp
        sample/sfz_support/sfz_import.cpp

        tuning/equal.cpp
        tuning/midikey_retuner.cpp

        infrastructure/file_map_view.cpp

        messaging/audio/audio_messages.cpp
        messaging/messaging.cpp

        modulation/group_matrix.cpp
        modulation/voice_matrix.cpp
        modulation/mod_curves.cpp
        modulation/modulator_storage.cpp

        selection/selection_manager.cpp

        voice/voice.cpp
        voice/preview_voice.cpp

        patch_io/patch_io.cpp

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
target_link_libraries(${PROJECT_NAME} PRIVATE fmt)
target_link_libraries(${PROJECT_NAME} PUBLIC
        eurorack

        sst-cpputils
        sst-filters
        sst-waveshapers
        sst-basic-blocks
        sst-effects
        sst-plugininfra
        sst-plugininfra::tinyxml
        sst-plugininfra::miniz
        sst-plugininfra::version_information
        sst-voicemanager

        libgig libakai
        mts-esp-client
        shortcircuit::simde
        shortcircuit::sqlite3
        taocpp::json

        FLAC++
        minimp3
        md5sum::md5

        scxt-resources-core

        sc-compiler-options
        )
