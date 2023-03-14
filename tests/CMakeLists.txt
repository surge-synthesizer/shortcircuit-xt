# And finally the test suite
add_executable(scxt-test
	test_main.cpp
        streaming.cpp)

target_link_libraries(scxt-test
        scxt-core
        shortcircuit::catch2
        )


