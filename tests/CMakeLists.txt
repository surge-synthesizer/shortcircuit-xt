# And finally the test suite
add_executable(scxt-test
		test_main.cpp
		console_ui.cpp
		test_utilities.cpp

		sfz_parse.cpp
        streaming.cpp
		sample_analytics.cpp
		processors_and_fx.cpp

		ui_basics.cpp

		selection_tests.cpp
)

target_link_libraries(scxt-test
		fmt
        scxt-core
        shortcircuit::catch2
        )

if (WIN32)
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
		target_link_libraries(scxt-test DbgHelp.lib)
	endif()
endif()


