project(sqlite3 VERSION 3.42.0 LANGUAGES C)

message(STATUS "Including local sqlite")
add_library(${PROJECT_NAME} sqlite3.c sqlite3.h)
add_library(shortcircuit::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE .)
target_compile_definitions(${PROJECT_NAME} PUBLIC
        SQLITE_OMIT_AUTHORIZATION=1
        SQLITE_OMIT_COMPILEOPTION_DIAGS=1
        SQLITE_OMIT_DEPRECATED=1
        SQLITE_OMIT_LOAD_EXTENSION=1
        SQLITE_OMIT_WAL=1)
