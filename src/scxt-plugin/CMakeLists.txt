project(scxt-plugin LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
        app/browser-ui/BrowserPane.cpp

        app/editor-impl/HasEditor.cpp
        app/editor-impl/SCXTEditor.cpp
        app/editor-impl/SCXTEditorMenus.cpp
        app/editor-impl/SCXTEditorResponseHandlers.cpp
        app/editor-impl/SCXTEditorDataCache.cpp
        app/editor-impl/KeyBindings.cpp

        app/edit-screen/EditScreen.cpp
        app/edit-screen/components/AdsrPane.cpp
        app/edit-screen/components/LFOPane.cpp
        app/edit-screen/components/MacroMappingVariantPane.cpp
        app/edit-screen/components/ModPane.cpp
        app/edit-screen/components/OutputPane.cpp
        app/edit-screen/components/PartEditScreen.cpp
        app/edit-screen/components/PartGroupSidebar.cpp
        app/edit-screen/components/ProcessorPane.cpp
        app/edit-screen/components/ProcessorPaneEQsFilters.cpp
        app/edit-screen/components/GroupSettingsCard.cpp
        app/edit-screen/components/GroupTriggersCard.cpp

        app/edit-screen/components/mapping-pane/MappingDisplay.cpp
        app/edit-screen/components/mapping-pane/MacroDisplay.cpp
        app/edit-screen/components/mapping-pane/VariantDisplay.cpp
        app/edit-screen/components/mapping-pane/SampleWaveform.cpp
        app/edit-screen/components/mapping-pane/ZoneLayoutDisplay.cpp
        app/edit-screen/components/mapping-pane/ZoneLayoutKeyboard.cpp

        app/missing-resolution/MissingResolutionScreen.cpp

        app/mixer-screen/MixerScreen.cpp
        app/mixer-screen/components/BusPane.cpp
        app/mixer-screen/components/ChannelStrip.cpp

        app/other-screens/AboutScreen.cpp
        app/other-screens/LogScreen.cpp
        app/other-screens/WelcomeScreen.cpp
        app/play-screen/PlayScreen.cpp

        app/shared/HeaderRegion.cpp
        app/shared/MenuValueTypein.cpp
        app/shared/PartSidebarCard.cpp
        app/shared/SingleMacroEditor.cpp
        app/shared/PartEffectsPane.cpp

        connectors/SCXTResources.cpp
        connectors/JsonLayoutEngineSupport.cpp

        theme/ColorMap.cpp
        theme/ThemeApplier.cpp

        clap/clap-helpers-config.cpp
        clap/scxt-plugin.cpp
        clap/scxt-clap-entry-impl.cpp
        )

message(STATUS "Globbing json-assets directory for cmrc")
file(GLOB_RECURSE scxt_json_sources
        LIST_DIRECTORIES false
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        json-assets/*
)
cmrc_add_resource_library(scxtui_json_layouts
        NAMESPACE scxtui_json_layouts
        ${scxt_json_sources}
)
set_target_properties(scxtui_json_layouts PROPERTIES UNITY_BUILD FALSE)

target_link_libraries(${PROJECT_NAME} PRIVATE
        fmt
        sst-plugininfra::strnatcmp
        sst-plugininfra
        scxt-resources
        scxtui_json_layouts
        clap_juce_shim
        clap_juce_shim_headers
        clap-wrapper-extensions
        clap-helpers
        scxt-core
        sst-jucegui
)
target_link_libraries(${PROJECT_NAME} PUBLIC clap)

if (NOT CMAKE_UNITY_BUILD)
        #message(STATUS "Includeing Melatonin")
        #target_link_libraries(${PROJECT_NAME} PUBLIC melatonin_inspector)
        #target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_MELATONIN_INSPECTOR=1)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        JUCE_STANDALONE_APPLICATION=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        )

target_include_directories(${PROJECT_NAME} PUBLIC .)
