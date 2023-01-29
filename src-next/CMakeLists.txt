project(scxt-next LANGUAGES CXX)

add_library(${PROJECT_NAME}-core
        dsp/generator.cpp
        dsp/sinc_table.cpp

        engine/engine.cpp

        sample/sample.cpp
        sample/sample_manager.cpp
        sample/loaders/load_riff_wave.cpp

        infrastructure/file_map_view.cpp

        voice/voice.cpp

        utils.cpp
        )
target_include_directories(${PROJECT_NAME}-core PUBLIC .)
target_link_libraries(${PROJECT_NAME}-core PUBLIC fmt sst-cpputils simde)

