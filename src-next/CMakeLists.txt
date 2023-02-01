project(scxt-next LANGUAGES CXX)

add_library(${PROJECT_NAME}-core STATIC
        dsp/generator.cpp
        dsp/sinc_table.cpp
        dsp/filter/filter.cpp

        dsp/filter/filter_supersvf.cpp
        dsp/filter/filter_oscillators.cpp

        engine/engine.cpp
        engine/zone.cpp
        engine/group.cpp
        engine/part.cpp

        sample/sample.cpp
        sample/sample_manager.cpp
        sample/loaders/load_riff_wave.cpp

        tuning/equal.cpp

        infrastructure/file_map_view.cpp

        vembertech/vt_dsp/basic_dsp.cpp
        vembertech/vt_dsp/lipol.cpp

        voice/voice.cpp

        utils.cpp
        )
target_include_directories(${PROJECT_NAME}-core PUBLIC .)
target_link_libraries(${PROJECT_NAME}-core PUBLIC fmt sst-cpputils sst-filters simde)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME}-core PUBLIC _USE_MATH_DEFINES WINDOWS=1)
endif()