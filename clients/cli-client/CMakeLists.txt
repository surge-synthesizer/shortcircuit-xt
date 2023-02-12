project(cli-client LANGUAGES CXX)

add_executable(cli-client main.cpp midi.cpp audio.cpp)
target_link_libraries(cli-client PUBLIC scxt-core rtmidi miniaudio)

if (UNIX AND NOT APPLE)
    target_link_libraries(cli-client PUBLIC dl)
endif()



