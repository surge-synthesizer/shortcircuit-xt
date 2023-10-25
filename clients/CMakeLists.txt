project(scxt-clients LANGUAGES CXX)


if (${SCXT_JUCE_CLASSIC_STYLE})
    add_subdirectory(juce-plugin)

    if(DEFINED ENV{ASIOSDK_DIR} OR BUILD_USING_MY_ASIO_LICENSE)
        if(BUILD_USING_MY_ASIO_LICENSE)
            message(STATUS "** BUILD USING OWN ASIO LICENSE **")
            message(STATUS "The resulting Shortcircuit XT standalone executable is not licensed for distribution!")
            message(STATUS "Fetching ASIO SDK...")

            set(ASIOSDK_DIR ${CMAKE_BINARY_DIR}/asio/asiosdk)
            add_custom_target(scxt-get-local-asio)
            add_custom_command(
                TARGET scxt-get-local-asio
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/asio
                COMMAND ${CMAKE_COMMAND} -D ASIO_SDK_DESTINATION=${CMAKE_BINARY_DIR}/asio -P ../cmake/get-asio.cmake
            )
            add_dependencies(scxt_plugin scxt-get-local-asio)
        else()
            file(TO_CMAKE_PATH "$ENV{ASIOSDK_DIR}" ASIOSDK_DIR)
            message(STATUS "ASIO SDK found at ${ASIOSDK_DIR}")
            message(STATUS "The resulting Shortcircuit XT standalone executable is not licensed for distribution!")
        endif()

        target_compile_definitions(scxt_plugin INTERFACE JUCE_ASIO=1)
        target_include_directories(scxt_plugin INTERFACE ${ASIOSDK_DIR}/common)
        set(JUCE_ASIO_SUPPORT TRUE)
    endif()
else()
    add_subdirectory(clap-first)
endif()