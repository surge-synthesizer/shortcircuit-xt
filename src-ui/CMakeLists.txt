project(scxt-ui LANGUAGES CXX)

juce_add_binary_data(scxt-ui-binary
        NAMESPACE scxt::ui::binary
        HEADER_NAME SCXTBinary.h
        SOURCES
        ../resources/fonts/Inter/static/Inter-Regular.ttf
        ../resources/fonts/Inter/static/Inter-Medium.ttf
        ../resources/fonts/Inter/static/Inter-Bold.ttf
        )

add_library(${PROJECT_NAME} STATIC

        components/SCXTEditor.cpp
        components/SCXTEditorResponseHandlers.cpp

        components/HeaderRegion.cpp
        components/MultiScreen.cpp
        components/SendFXScreen.cpp

        components/multi/AdsrPane.cpp
        components/multi/PartGroupSidebar.cpp

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