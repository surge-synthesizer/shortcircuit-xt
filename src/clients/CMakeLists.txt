project(scxt-clients LANGUAGES CXX)

add_subdirectory(console-ui)
add_subdirectory(clap-first)
add_subdirectory(cli-tools)
add_subdirectory(stress-tests)
add_dependencies(cli-tools make-stress)
