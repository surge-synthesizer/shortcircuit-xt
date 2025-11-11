project(stress-tests)

add_executable(make-stress make-stress.cpp)
target_link_libraries(make-stress
        scxt-core
        fmt
        console-ui
)