# And finally the test suite
add_executable(sc3-test
        lowlevel_io.cpp
        mmio_test.cpp
        test_main.cpp
        format_tests.cpp
        vt_miscellany.cpp
        config_test.cpp
        logging_test.cpp
        profiler_test.cpp
        zone_tests.cpp filesystem_basics.cpp)

target_link_libraries(sc3-test
        shortcircuit-core
        shortcircuit::catch2
        )
