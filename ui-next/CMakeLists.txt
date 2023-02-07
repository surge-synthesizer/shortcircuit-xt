project(scxt-ui LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC

        components/SCXTEditor.cpp
        components/HeaderRegion.cpp
        components/MultiScreen.cpp
        components/SendFXScreen.cpp

        )

target_link_libraries(${PROJECT_NAME} PUBLIC
        scxt-next-core
        sst-jucegui
        juce::juce_gui_basics
        )

target_compile_definitions(${PROJECT_NAME} PRIVATE JUCE_STANDALONE_APPLICATION=0)
target_include_directories(${PROJECT_NAME} PUBLIC .)