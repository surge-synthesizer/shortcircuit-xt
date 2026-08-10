# And finally the test suite
add_executable(scxt-test
		test_main.cpp

		sfz_parse.cpp
        streaming.cpp
		sample_analytics.cpp
		sample_load.cpp
		sample_fuzz.cpp
		processors_and_fx.cpp

		ui_basics.cpp

		selection_tests.cpp
		modify_structure.cpp
		modulator_tests.cpp
		mod_matrix_operations_tests.cpp
		exclusive_group_tests.cpp
		keyswitch_tests.cpp
		group_trigger_tests.cpp
		glide_tests.cpp
		legato_tests.cpp
		undo_tests.cpp
		importer_tests.cpp
		midi_channel_tests.cpp
		macro_id_tests.cpp
		tuning_tests.cpp
		variant_edit_all_tests.cpp
)

target_compile_definitions(scxt-test PRIVATE
		SCXT_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

# Tests don't unity build. Concatenating them makes each file's file scope
# visible to the next, which turns a helper or an include in one test into a
# collision in an unrelated one.
set_target_properties(scxt-test PROPERTIES UNITY_BUILD OFF)

target_link_libraries(scxt-test
		fmt
        scxt-core
        shortcircuit::catch2
		console-ui
        )

if (WIN32)
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
		target_link_libraries(scxt-test DbgHelp.lib)
	endif()
endif()


