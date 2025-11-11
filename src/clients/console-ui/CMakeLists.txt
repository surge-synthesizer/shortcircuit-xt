project(console-ui)

add_library(${PROJECT_NAME} STATIC console_ui.cpp console_harness.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC .)
target_link_libraries(${PROJECT_NAME} PRIVATE scxt-core fmt)