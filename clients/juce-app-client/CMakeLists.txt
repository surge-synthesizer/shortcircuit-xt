project(juce-app-client VERSION 0.9 LANGUAGES CXX)

juce_add_gui_app(${PROJECT_NAME}
        BUNDLE_ID org.surge-synth-team.scxt-next.juce-app-client
        NEEDS_CURL                      FALSE
        NEEDS_WEB_BROWSER               FALSE
        MICROPHONE_PERMISSION_ENABLED   TRUE
        CAMERA_PERMISSION_ENABLED       FALSE
        BLUETOOTH_PERMISSION_ENABLED    TRUE
        FILE_SHARING_ENABLED            FALSE
        DOCUMENT_BROWSER_ENABLED        FALSE
        REQUIRES_FULL_SCREEN            FALSE
        )

target_sources(${PROJECT_NAME} PUBLIC
        juce-app-client-main.cpp
        audio.cpp
        midi.cpp
        )

target_compile_definitions(${PROJECT_NAME} PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        )

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        scxt-core
        scxt-ui

        rtmidi
        miniaudio

        juce::juce_gui_basics)