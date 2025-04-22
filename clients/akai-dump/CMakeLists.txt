
project(akai-dump)

add_executable(${PROJECT_NAME} akai-dump.cpp)

target_link_libraries(${PROJECT_NAME}
        scxt-core
        shortcircuit::catch2
        fmt
        )


