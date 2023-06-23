project(scxt-ui LANGUAGES CXX)

juce_add_binary_data(scxt-ui-binary
        NAMESPACE scxt::ui::binary
        HEADER_NAME SCXTBinary.h
        SOURCES
        ../resources/fonts/Inter/static/Inter-Regular.ttf
        ../resources/fonts/Inter/static/Inter-Medium.ttf
        ../resources/fonts/Inter/static/Inter-Bold.ttf
        ../resources/fonts/Anonymous_Pro/AnonymousPro-Regular.ttf
        ../resources/images/SCicon.svg
        )

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

        components/widgets/ShortCircuitMenuButton.cpp
        components/widgets/Tooltip.cpp

        connectors/SCXTStyleSheetCreator.cpp

        )


target_link_libraries(${PROJECT_NAME} PUBLIC
        scxt-core
        scxt-ui-binary
        sst-jucegui
        juce::juce_gui_basics
        )

target_compile_definitions(${PROJECT_NAME} PRIVATE JUCE_STANDALONE_APPLICATION=0)
target_include_directories(${PROJECT_NAME} PUBLIC .)