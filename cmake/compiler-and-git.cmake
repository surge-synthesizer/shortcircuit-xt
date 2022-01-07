# Set up version information using the same approach as surge, namely
# with an external cmake run, a git-info target, and a generated
# ${bld}/geninclude/version.cpp
execute_process(
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND}
        -D PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        -D PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        -D SHORTCSRC=${CMAKE_SOURCE_DIR}
        -D SHORTCBLD=${CMAKE_BINARY_DIR}
        -D AZURE_PIPELINE=${AZURE_PIPELINE}
        -D WIN32=${WIN32}
        -P ${CMAKE_SOURCE_DIR}/cmake/versiontools.cmake
)
add_custom_target(git-info BYPRODUCTS ${CMAKE_BINARY_DIR}/geninclude/version.cpp
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND}
        -D PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        -D PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        -D SHORTCSRC=${CMAKE_SOURCE_DIR}
        -D SHORTCBLD=${CMAKE_BINARY_DIR}
        -D AZURE_PIPELINE=${AZURE_PIPELINE}
        -D WIN32=${WIN32}
        -P ${CMAKE_SOURCE_DIR}/cmake/versiontools.cmake
        )

# Platform Specific Compile Settings
add_library(sc-compiler-options INTERFACE)
if (APPLE)
    set(OS_COMPILE_OPTIONS
            -Wno-invalid-source-encoding
            -Wno-undefined-bool-conversion
            -Wno-format-security
            -Wno-writable-strings

            -fvisibility=hidden -fvisibility-inlines-hidden
            )
    set(OS_COMPILE_DEFINITIONS MAC=1)

    set(OS_LINK_LIBRARIES
            "-framework CoreServices"
            )
elseif (UNIX AND NOT APPLE)
    set(OS_COMPILE_OPTIONS
            -Wno-multichar
            -fPIC
            -fvisibility=hidden -fvisibility-inlines-hidden
            )
    set(OS_COMPILE_DEFINITIONS
            LINUX=1)

    set(OS_LINK_LIBRARIES
            )
else ()
    if (SCXT_INCLUDE_ASIO_SUPPORT)
        execute_process(COMMAND powershell
                -ExecutionPolicy Bypass
                -File "scripts/win_dl_asiosdk.ps1"
                )
    endif ()
    set(OS_COMPILE_OPTIONS
            /wd4244   # convert float from double
            /wd4305   # truncate from double to float
            /wd4018   # signed unsigned compare
            /wd4996   # don't warn on stricmp vs _stricmp and other posix names
            /wd4267   # converting size_t <-> int
            )
    set(OS_COMPILE_DEFINITIONS
            WIN=1
            WINDOWS=1
            _CRT_SECURE_NO_WARNINGS=1
            )

    set(OS_LINK_LIBRARIES
            shell32
            user32

            Winmm
            gdi32
            gdiplus
            msimg32

            ComDlg32
            ComCtl32
            )
endif ()
target_compile_options(sc-compiler-options INTERFACE ${OS_COMPILE_OPTIONS})
target_compile_definitions(sc-compiler-options INTERFACE ${OS_COMPILE_DEFINITIONS} TARGET_HEADLESS=1)
target_link_libraries(sc-compiler-options INTERFACE ${OS_LINK_LIBRARIES})
add_dependencies(sc-compiler-options git-info)
add_library(shortcircuit::compiler-options ALIAS sc-compiler-options)
