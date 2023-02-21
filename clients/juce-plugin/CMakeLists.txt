project(scxt_plugin VERSION 0.9.0 LANGUAGES CXX)

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
    
    FORMATS Standalone # AU VST3

    COPY_PLUGIN_AFTER_BUILD TRUE
)

#clap_juce_extensions_plugin(TARGET ${PROJECT_NAME}
#        CLAP_ID "org.surge-synth-team.shortcircuit-xt"
#        CLAP_FEATURES instrument sampler)

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

    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0

    JUCE_ALSA=1
    JUCE_JACK=1
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

