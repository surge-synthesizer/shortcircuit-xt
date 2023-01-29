project(cli-client LANGUAGES CXX)

add_executable(cli-client main.cpp)
target_link_libraries(cli-client PUBLIC scxt-next-core)



