project(scxt-next LANGUAGES CXX)

add_library(${PROJECT_NAME}-core STATIC
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

if (WIN32)
    target_compile_definitions(${PROJECT_NAME}-core PUBLIC _USE_MATH_DEFINES)
endif()