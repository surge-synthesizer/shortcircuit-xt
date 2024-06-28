
project(scxt-sfz-token-dump)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
        scxt-core
        shortcircuit::catch2
        )


