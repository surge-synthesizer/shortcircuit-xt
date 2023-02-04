project(scxt-next LANGUAGES CXX)

add_library(${PROJECT_NAME}-core STATIC
        dsp/generator.cpp
        dsp/sinc_table.cpp
        dsp/processor/processor.cpp

        dsp/processor/processor_supersvf.cpp
        dsp/processor/processor_oscillators.cpp

        engine/engine.cpp
        engine/zone.cpp
        engine/group.cpp
        engine/part.cpp

        json/stream.cpp

        sample/sample.cpp
        sample/sample_manager.cpp
        sample/loaders/load_riff_wave.cpp

        tuning/equal.cpp

        infrastructure/file_map_view.cpp

        modulation/voice_matrix.cpp
        modulation/modulators/steplfo.cpp

        vembertech/vt_dsp/basic_dsp.cpp
        vembertech/vt_dsp/lipol.cpp

        voice/voice.cpp

        utils.cpp
        )
target_include_directories(${PROJECT_NAME}-core PUBLIC .)
target_link_libraries(${PROJECT_NAME}-core PUBLIC
        fmt
        sst-cpputils sst-filters
        shortcircuit::simde
        shortcircuit::readerwriterqueue
        taocpp::json)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME}-core PUBLIC _USE_MATH_DEFINES WINDOWS=1)
else()
    target_compile_options(${PROJECT_NAME}-core PUBLIC -Werror)
endif()