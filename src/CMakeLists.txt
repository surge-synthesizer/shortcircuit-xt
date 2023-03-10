project(scxt-core LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
        datamodel/parameter.cpp

        dsp/generator.cpp
        dsp/data_tables.cpp
        dsp/processor/processor.cpp

        dsp/processor/processor_supersvf.cpp
        dsp/processor/processor_oscillators.cpp
        dsp/processor/processor_distortions.cpp

        engine/engine.cpp
        engine/zone.cpp
        engine/group.cpp
        engine/part.cpp
        engine/memory_pool.cpp

        json/stream.cpp

        sample/sample.cpp
        sample/sample_manager.cpp
        sample/loaders/load_riff_wave.cpp

        tuning/equal.cpp

        infrastructure/file_map_view.cpp

        messaging/audio/audio_messages.cpp
        messaging/messaging.cpp

        modulation/voice_matrix.cpp
        modulation/modulators/steplfo.cpp

        selection/selection_manager.cpp

        vembertech/vt_dsp/basic_dsp.cpp
        vembertech/vt_dsp/lipol.cpp

        voice/voice.cpp

        utils.cpp

        ${CMAKE_BINARY_DIR}/geninclude/version.cpp
        )
target_include_directories(${PROJECT_NAME} PUBLIC .)
target_link_libraries(${PROJECT_NAME} PUBLIC
        fmt
        libgig
        libakai
        sst-cpputils sst-filters sst-basic-blocks
        shortcircuit::simde
        shortcircuit::readerwriterqueue
        taocpp::json)

target_compile_definitions(${PROJECT_NAME} PUBLIC  $<IF:$<CONFIG:DEBUG>,BUILD_IS_DEBUG,BUILD_IS_RELEASE>=1)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _USE_MATH_DEFINES WINDOWS=1)
else()
    target_compile_options(${PROJECT_NAME} PUBLIC -Werror)
endif()