# Set up version information using the same approach as surge, namely
# with an external cmake run, a git-info target, and a generated
# ${bld}/geninclude/version.cpp

add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/geninclude/version.cpp
        DEPENDS ${CMAKE_SOURCE_DIR}/cmake/version.h
            ${CMAKE_SOURCE_DIR}/cmake/version.cpp.in
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND}
            -D PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
            -D PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
            -D SHORTCSRC=${CMAKE_SOURCE_DIR}
            -D SHORTCBLD=${CMAKE_BINARY_DIR}
            -D AZURE_PIPELINE=${AZURE_PIPELINE}
            -D WIN32=${WIN32}
            -D CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID}
            -D CMAKE_CXX_COMPILER_VERSION=${CMAKE_CXX_COMPILER_VERSION}
            -P ${CMAKE_SOURCE_DIR}/cmake/versiontools.cmake
        )

# Platform Specific Compile Settings
add_library(sc-compiler-options)

target_sources(sc-compiler-options PRIVATE ${CMAKE_BINARY_DIR}/geninclude/version.cpp)

target_include_directories(sc-compiler-options PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

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
            -march=nehalem
            )
    set(OS_COMPILE_DEFINITIONS
            LINUX=1)

    set(OS_LINK_LIBRARIES
            )
else ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(OS_COMPILE_OPTIONS
                /arch:SSE42
                /wd4244   # convert float from double
                /wd4305   # truncate from double to float
                /wd4018   # signed unsigned compare
                /wd4996   # don't warn on stricmp vs _stricmp and other posix names
                /wd4267   # converting size_t <-> int
                )
    endif ()

    if (WIN32)
        # Win32 clang marks strncpy as deprecatred and wants strncpy_s; maybe one day
        # fix that but not today
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set(OS_COMPILE_OPTIONS
                -Wno-multichar
                -march=nehalem

                # Targeting Windows with GCC/Clang is experimental
                # $<$<NOT:$<BOOL:${WIN32}>>:-Werror>

                # PE/COFF doesn't support visibility
                $<$<NOT:$<BOOL:${WIN32}>>:-fvisibility=hidden>

                # Inlines visibility is only relevant with C++
                $<$<AND:$<NOT:$<BOOL:${WIN32}>>,$<COMPILE_LANGUAGE:CXX>>:-fvisibility-inlines-hidden>)
    endif ()

    set(OS_COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS=1 WINDOWS=1 _USE_MATH_DEFINES=1)
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
target_compile_definitions(sc-compiler-options INTERFACE ${OS_COMPILE_DEFINITIONS})
target_include_directories(sc-compiler-options INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
target_link_libraries(sc-compiler-options INTERFACE ${OS_LINK_LIBRARIES})
add_library(shortcircuit::compiler-options ALIAS sc-compiler-options)
