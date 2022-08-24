add_subdirectory(${CMAKE_SOURCE_DIR}/libs/sst/sst-jucegui sst-jucegui)

# The JUCE plugin Shortcircuit XT has an AU, VST3 and standalone version
if (WIN32)
    set(SCXT_COPY_AFTER FALSE)
else ()
    set(SCXT_COPY_AFTER TRUE)
endif ()

list(APPEND SCXT_FORMATS VST3)
list(APPEND SCXT_FORMATS AU)
list(APPEND SCXT_FORMATS Standalone)
if (BUILD_LV2)
    message(STATUS "Activating LV2 for testing purpose")
    list(APPEND SCXT_FORMATS LV2)
endif ()

message(STATUS "Building Shortcircuit in formats: ${SCXT_FORMATS}")

juce_add_plugin(ShortcircuitXT
        PRODUCT_NAME "Shortcircuit XT"
        COMPANY_NAME "Surge Synth Team"
        BUNDLE_ID "org.surge-synth-team.shortcircuit-xt"
        PLUGIN_MANUFACTURER_CODE VmbA
        PLUGIN_CODE ScXT

        LV2URI https://surge-synthesizer.github.io/lv2/ShortcircuitXT

        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE

        ICON_BIG "${CMAKE_SOURCE_DIR}/resources/images/SCAppIcon.png"

        FORMATS ${SCXT_FORMATS}

        COPY_PLUGIN_AFTER_BUILD ${SCXT_COPY_AFTER}
        )

# images used in the ui
juce_add_binary_data(UIAssets
        NAMESPACE SCXTUIAssets
        HEADER_NAME BinaryUIAssets.h
        SOURCES
        ${CMAKE_SOURCE_DIR}/resources/images/wavehandle_start.png
        ${CMAKE_SOURCE_DIR}/resources/images/wavehandle_end.png
        ${CMAKE_SOURCE_DIR}/resources/images/wavehandle_crossfade.png
        ${CMAKE_SOURCE_DIR}/resources/images/SCicon.svg

        ${CMAKE_SOURCE_DIR}/resources/fonts/Anonymous_Pro/AnonymousPro-Regular.ttf
        ${CMAKE_SOURCE_DIR}/resources/fonts/Anonymous_Pro/AnonymousPro-Bold.ttf
        )

clap_juce_extensions_plugin(TARGET ShortcircuitXT
        CLAP_ID "org.surge-synth-team.shortcircuit-xt"
        CLAP_FEATURES "instrument" "sampler")

set_target_properties(UIAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)


# Then set up the source and the JUCE settings
target_sources(ShortcircuitXT PRIVATE
        SCXTProcessor.cpp
        SCXTEditor.cpp
        SCXTLookAndFeel.cpp

        components/BrowserSidebar.cpp
        components/HeaderPanel.cpp
        components/WaveDisplay.cpp
        components/DebugPanel.cpp
        components/ZoneKeyboardDisplay.cpp

        style/StyleSheet.cpp

        widgets/ParamEditor.cpp

        pages/FXPage.cpp
        pages/PartPage.cpp
        pages/ZonePage.cpp
        )

target_include_directories(ShortcircuitXT PRIVATE .)
target_compile_definitions(ShortcircuitXT PUBLIC
        JUCE_ALLOW_STATIC_NULL_VARIABLES=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_STRICT_REFCOUNTEDPOINTER=1

        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0

        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0

        JUCE_ALSA=1
        JUCE_JACK=1
        )

target_link_libraries(ShortcircuitXT PRIVATE
        UIAssets
        shortcircuit-core
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        sst-jucegui
        )


set_property(TARGET ShortcircuitXT PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET ShortcircuitXT PROPERTY VISIBILITY_INLINES_HIDDEN ON)

