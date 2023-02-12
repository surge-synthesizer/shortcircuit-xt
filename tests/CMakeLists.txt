# And finally the test suite
add_executable(sc3-test
	test_main.cpp
        streaming.cpp)

target_link_libraries(sc3-test
        scxt-core
        shortcircuit::catch2
        )


