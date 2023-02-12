project(scxt-ui LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC

        components/SCXTEditor.cpp
        components/SCXTEditorResponseHandlers.cpp

        components/HeaderRegion.cpp
        components/MultiScreen.cpp
        components/SendFXScreen.cpp

        components/multi/AdsrPane.cpp

        )

target_link_libraries(${PROJECT_NAME} PUBLIC
        scxt-core
        sst-jucegui
        juce::juce_gui_basics
        )

target_compile_definitions(${PROJECT_NAME} PRIVATE JUCE_STANDALONE_APPLICATION=0)
target_include_directories(${PROJECT_NAME} PUBLIC .)