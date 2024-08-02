project(scxt-ui LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC

        components/SCXTEditor.cpp
        components/SCXTEditorMenus.cpp
        components/SCXTEditorResponseHandlers.cpp

        components/HasEditor.cpp

        components/HeaderRegion.cpp
        components/MixerScreen.cpp
        components/MultiScreen.cpp
        components/SendFXScreen.cpp
        components/AboutScreen.cpp
        components/LogScreen.cpp

        components/browser/BrowserPane.cpp

        components/mixer/BusPane.cpp
        components/mixer/PartEffectsPane.cpp
        components/mixer/ChannelStrip.cpp

        components/multi/AdsrPane.cpp
        components/multi/LFOPane.cpp
        components/multi/MappingPane.cpp
        components/multi/ModPane.cpp
        components/multi/OutputPane.cpp
        components/multi/PartGroupSidebar.cpp
        components/multi/ProcessorPane.cpp
        components/multi/ProcessorPaneEQsFilters.cpp

        connectors/SCXTResources.cpp
        connectors/JSONLayoutConsumer.cpp

        theme/ColorMap.cpp
        theme/ThemeApplier.cpp

        )

message(STATUS "Globbing json-layout directory for cmrc")
file(GLOB_RECURSE scxt_json_sources
        LIST_DIRECTORIES false
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        json-layout/*.json
)
cmrc_add_resource_library(scxtui_json_layouts
        NAMESPACE scxtui_json_layouts
        ${scxt_json_sources}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
        scxt-core
        scxt-resources
        sst-jucegui
        juce::juce_gui_basics

        sst-plugininfra::strnatcmp
        melatonin_inspector

        scxtui_json_layouts
        )

target_compile_definitions(${PROJECT_NAME} PRIVATE
        JUCE_STANDALONE_APPLICATION=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        )

target_include_directories(${PROJECT_NAME} PUBLIC .)
