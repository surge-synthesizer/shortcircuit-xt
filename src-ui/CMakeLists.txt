project(scxt-ui LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
        app/browser-ui/BrowserPane.cpp

        app/editor-impl/HasEditor.cpp
        app/editor-impl/SCXTEditor.cpp
        app/editor-impl/SCXTEditorMenus.cpp
        app/editor-impl/SCXTEditorResponseHandlers.cpp

        app/edit-screen/EditScreen.cpp
        app/edit-screen/components/AdsrPane.cpp
        app/edit-screen/components/LFOPane.cpp
        app/edit-screen/components/MappingPane.cpp
        app/edit-screen/components/ModPane.cpp
        app/edit-screen/components/OutputPane.cpp
        app/edit-screen/components/PartGroupSidebar.cpp
        app/edit-screen/components/ProcessorPane.cpp
        app/edit-screen/components/ProcessorPaneEQsFilters.cpp

        app/mixer-screen/MixerScreen.cpp
        app/mixer-screen/components/BusPane.cpp
        app/mixer-screen/components/ChannelStrip.cpp
        app/mixer-screen/components/PartEffectsPane.cpp

        app/other-screens/AboutScreen.cpp
        app/other-screens/LogScreen.cpp
        app/other-screens/WelcomeScreen.cpp
        app/play-screen/PlayScreen.cpp

        app/shared/HeaderRegion.cpp
        app/shared/PartSidebarCard.cpp
        app/shared/SingleMacroEditor.cpp

        connectors/SCXTResources.cpp
        connectors/JSONAssetSupport.cpp

        theme/ColorMap.cpp
        theme/ThemeApplier.cpp

        )

message(STATUS "Globbing json-assets directory for cmrc")
file(GLOB_RECURSE scxt_json_sources
        LIST_DIRECTORIES false
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        json-assets/*.json
)
cmrc_add_resource_library(scxtui_json_layouts
        NAMESPACE scxtui_json_layouts
        ${scxt_json_sources}
)
set_target_properties(scxtui_json_layouts PROPERTIES UNITY_BUILD FALSE)


target_link_libraries(${PROJECT_NAME} PUBLIC
        scxt-core
        scxt-resources
        sst-jucegui

        sst-plugininfra::strnatcmp

        scxtui_json_layouts
        )

if (NOT CMAKE_UNITY_BUILD)
        message(STATUS "Includeing Melatonin")
        target_link_libraries(${PROJECT_NAME} PUBLIC melatonin_inspector)
        target_compile_definitions(${PROJECT_NAME} PUBLIC HAS_MELATONIN_INSPECTOR=1)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        JUCE_STANDALONE_APPLICATION=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        )

target_include_directories(${PROJECT_NAME} PUBLIC .)
