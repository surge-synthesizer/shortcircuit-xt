project(init-maker)

message(STATUS "Making ${PROJECT_NAME} executable")
add_executable(${PROJECT_NAME} init-maker.cpp)
target_link_libraries(${PROJECT_NAME} scxt-core)