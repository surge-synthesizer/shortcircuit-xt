
add_custom_target(code-quality-pipeline-checks)

# Clang Format checks
set(CLANG_FORMAT_DIRS src wrappers tests)
set(CLANG_FORMAT_EXTS cpp h)
foreach (dir ${CLANG_FORMAT_DIRS})
    foreach (ext ${CLANG_FORMAT_EXTS})
        list(APPEND CLANG_FORMAT_GLOBS "': (glob) ${dir}/**/*.${ext}' ")
    endforeach ()
endforeach ()
add_custom_command(TARGET code-quality-pipeline-checks
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo About to check clang-format
        COMMAND git ls-files -- ${CLANG_FORMAT_GLOBS} | xargs clang-format --dry-run --Werror
        )
