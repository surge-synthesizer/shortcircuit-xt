
project(scxt_plugin VERSION ${ShortcircuitXT_VERSION} LANGUAGES CXX)

message(STATUS "Plugin Version number is ${PROJECT_VERSION}")

if (SCXT_BUILD_VST3)
    list(APPEND SCXT_FORMATS VST3)
endif ()
if (SCXT_BUILD_AU AND APPLE AND NOT CMAKE_CROSSCOMPILING)
    list(APPEND SCXT_FORMATS AU)
endif ()
if (SCXT_BUILD_STANDALONE)
    list(APPEND SCXT_FORMATS Standalone)
endif ()

message(STATUS "JUCE plugin layer building ${SCXT_FORMATS}")

juce_add_plugin(${PROJECT_NAME}
        PRODUCT_NAME "Shortcircuit XT"
        COMPANY_NAME "Surge Synth Team"
        BUNDLE_ID "org.surge-synth-team.shortcircuit-xt"
        PLUGIN_MANUFACTURER_CODE VmbA
        PLUGIN_CODE ScXT

        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE

        ICON_BIG "${CMAKE_SOURCE_DIR}/resources/images/SCAppIcon.png"

        FORMATS ${SCXT_FORMATS}

        VST3_AUTO_MANIFEST FALSE

        COPY_PLUGIN_AFTER_BUILD ${SCXT_COPY_PLUGIN_AFTER_BUILD}
        )

if (SCXT_BUILD_CLAP)
    clap_juce_extensions_plugin(TARGET ${PROJECT_NAME}
            CLAP_ID "org.surge-synth-team.shortcircuit-xt"
            CLAP_FEATURES instrument sampler)
endif ()

if (JUCE_ASIO_SUPPORT)
    target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_ASIO=1)
endif ()

target_include_directories(${PROJECT_NAME}
        PRIVATE
        .
        )

target_sources(${PROJECT_NAME} PRIVATE
        SCXTProcessor.cpp SCXTPluginEditor.cpp)

target_compile_definitions(${PROJECT_NAME} PUBLIC
        JUCE_ALLOW_STATIC_NULL_VARIABLES=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1

        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUUCE_USE_CAMERA=disabled

        JUCE_MODAL_LOOPS_PERMITTED=0
        JUCE_COREGRAPHICS_DRAW_ASYNC=1
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0

        JUCE_ALSA=$<IF:$<BOOL:${SCXT_USE_ALSA}>,1,0>
        JUCE_JACK=$<IF:$<BOOL:${SCXT_USE_JACK}>,1,0>

        JUCE_USE_FLAC=0   # we get FLAC through xiph directly in the pre-juce CORE

        JUCE_WASAPI=$<IF:$<BOOL:${SCXT_BUILD_STANDALONE}>,1,0>
        JUCE_DIRECTSOUND=$<IF:$<BOOL:${SCXT_BUILD_STANDALONE}>,1,0>

        JUCE_CATCH_UNHANDLED_EXCEPTIONS=0
        )

target_link_libraries(${PROJECT_NAME} PUBLIC
        juce::juce_graphics
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_audio_devices

        scxt-core
        scxt-ui

        sst-plugininfra
        )

