# And finally the test suite
add_executable(scxt-test
	test_main.cpp
		sfz_parse.cpp
        streaming.cpp
		sample_analytics.cpp
		processors_and_fx.cpp

		console_ui.cpp
		ui_basics.cpp
)

target_link_libraries(scxt-test
		fmt
        scxt-core
        shortcircuit::catch2
        )

if (WIN32)
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
		target_link_libraries(scxt-test PRIVATE DbgHelp.lib)
	endif()
endif()


