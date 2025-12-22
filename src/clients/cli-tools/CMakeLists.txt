project(cli-tools)

add_custom_target(${PROJECT_NAME})
function (add_cli_tool)
    set(oneValueArgs SOURCE)
    cmake_parse_arguments(CLI "" "${oneValueArgs}" "" ${ARGN})

    add_executable(${CLI_SOURCE} ${CLI_SOURCE}.cpp)
    target_link_libraries(${CLI_SOURCE}
            scxt-core
            fmt
    )
    add_dependencies(${PROJECT_NAME} ${CLI_SOURCE})
endfunction()

add_cli_tool(SOURCE akai-dump)
add_cli_tool(SOURCE exs-dump)
add_cli_tool(SOURCE init-maker)
add_cli_tool(SOURCE riff-dump)
add_cli_tool(SOURCE sfz-token-dump)
add_cli_tool(SOURCE scpscm-to-json)
