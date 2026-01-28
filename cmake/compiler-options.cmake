
# Calculate bitness
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
if (NOT ${BITS} EQUAL 64)
    message(WARNING "${PROJECT_NAME} has only been tested on 64 bits. This may not work")
endif ()


# Everything here is C++ 17 now
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND UNIX AND NOT APPLE AND NOT SCXT_SKIP_PIE_CHANGE)
    message(STATUS "Setting -no-pie on EXE flags; use SCXT_SKIP_PIE_CHANGE=TRUE to avoid")
    set(CMAKE_EXE_LINKER_FLAGS "-no-pie")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    if (${SCXT_SANITIZE})
        message(STATUS "Sanitizer is ON")
    endif ()

    # BP note: If you want to turn on llvm/gcc sanitize, remove this and the link options below
    add_compile_options(
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=address>
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=undefined>
    )

    add_link_options(
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=address>
            $<$<BOOL:${SCXT_SANITIZE}>:-fsanitize=undefined>
    )
endif ()


# Platform Specific Compile Settings
add_library(sc-compiler-options INTERFACE)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Never want shared if not specified")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

if (${BUILD_SHARED_LIBS})
    message(FATAL_ERROR "You have overriden BUILD_SHARED_LIBS to be ON. This is an unsupported configuration")
endif()

if (APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
    set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
    if( ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER_EQUAL "15.0.0" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "15.1")
        add_link_options(-Wl,-ld_classic)
        add_compile_definitions(JUCE_SILENCE_XCODE_15_LINKER_WARNING=1)
    endif()
endif ()


if (APPLE)
    set(OS_COMPILE_OPTIONS
            -Werror
            -Wno-multichar
            -Wno-shorten-64-to-32
            )
    set(OS_COMPILE_DEFINITIONS MAC=1)

    set(OS_LINK_LIBRARIES
            "-framework CoreServices"
            "-framework Security" # for MD5
            )
elseif (UNIX AND NOT APPLE)
    set(OS_COMPILE_OPTIONS
            -Werror
            -Wno-multichar
            -fPIC  # probably not needed with CMAKE_POSITION_INDEPENDENT_CODE_ON but hey
            $<IF:$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},aarch64>,-march=armv8-a,-march=nehalem>
            )
    set(OS_COMPILE_DEFINITIONS
            LINUX=1)

    set(OS_LINK_LIBRARIES
            )
else ()

    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
        set(OS_COMPILE_OPTIONS
                /bigobj
                /wd4244   # convert float from double
                /wd4305   # truncate from double to float
                /wd4018   # signed unsigned compare
                /wd4996   # don't warn on stricmp vs _stricmp and other posix names
                /wd4267   # converting size_t <-> int
                /wd4005   # macro redefinition for nominmax
                /utf-8
                )
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # We can set clang-cl options here if we want
            # list(APPEND OS_COMPILE_OPTIONS )
        endif()
        message(STATUS "MSVC flags are ${OS_COMPILE_OPTIONS}")
    endif ()

    if (WIN32)
        # Win32 clang marks strncpy as deprecatred and wants strncpy_s; maybe one day
        # fix that but not today
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set(OS_COMPILE_OPTIONS
                -Wno-multichar
                -Wno-macro-redefined
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
target_compile_definitions(sc-compiler-options INTERFACE $<IF:$<CONFIG:DEBUG>,BUILD_IS_DEBUG,BUILD_IS_RELEASE>=1)
target_include_directories(sc-compiler-options INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
target_link_libraries(sc-compiler-options INTERFACE ${OS_LINK_LIBRARIES})
add_dependencies(sc-compiler-options version-info)
add_library(shortcircuit::compiler-options ALIAS sc-compiler-options)
