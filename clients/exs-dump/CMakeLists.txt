
project(exs-dump)

add_executable(${PROJECT_NAME} exs-dump.cpp)

target_link_libraries(${PROJECT_NAME}
        scxt-core
        shortcircuit::catch2
        fmt
        )


