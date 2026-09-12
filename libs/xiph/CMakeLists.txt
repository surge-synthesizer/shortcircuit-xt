if (${SCXT_USE_FLAC})
    set(BUILD_PROGRAMS OFF CACHE BOOL "Build and install programs" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build and install examples" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
    set(BUILD_DOCS OFF CACHE BOOL "Build and install doxygen documents" FORCE)
    set(WITH_FORTIFY_SOURCE OFF CACHE BOOL "Enable protection against buffer overflows" FORCE)
    set(WITH_STACK_PROTECTOR OFF CACHE BOOL "Enable GNU GCC stack smash protection" FORCE)
    set(INSTALL_MANPAGES OFF CACHE BOOL "Install MAN pages" FORCE)
    set(INSTALL_PKGCONFIG_MODULES OFF CACHE BOOL "Install PkgConfig modules" FORCE)
    set(INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "Install CMake package-config module" FORCE)
    set(WITH_OGG OFF CACHE BOOL "ogg support (default: test for libogg)" FORCE)
    set(WITH_AVX OFF CACHE BOOL "scxt is sse4.2 not ACXX" FORCE)
    if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "arm64ec")
        # arm64ec defines _M_X64, so flac picks its x86 cpuid and sse paths
        set(WITH_ASM OFF CACHE BOOL "No x86 intrinsics on arm64ec" FORCE)
    endif()

    add_subdirectory(flac EXCLUDE_FROM_ALL)
    target_compile_definitions(FLAC++ PUBLIC SCXT_USE_FLAC=1)


    if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # Flac makes a lot of warnings which are somebody else's problem
            target_compile_options(FLAC PRIVATE -w)
            target_compile_options(FLAC++ PRIVATE -w)
        endif()
    endif()
else ()
    add_library(FLAC INTERFACE)
    add_library(FLAC++ INTERFACE)
endif ()

if (${SCXT_USE_OPUS})
    # ogg + opus (libopus) build with their own CMake; opusfile ships no CMake so
    # we build its (non-http) core ourselves below.
    set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
    set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
    if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "arm64ec")
        # CMAKE_SYSTEM_PROCESSOR reads AMD64 here so opus builds its celt/x86 tree
        set(OPUS_DISABLE_INTRINSICS ON CACHE BOOL "No x86 intrinsics on arm64ec" FORCE)
    endif()

    add_subdirectory(ogg EXCLUDE_FROM_ALL)
    add_subdirectory(opus EXCLUDE_FROM_ALL)

    # opusfile core - skip src/http.c so we don't pull in OpenSSL
    add_library(opusfile STATIC
        opusfile/src/info.c
        opusfile/src/internal.c
        opusfile/src/opusfile.c
        opusfile/src/stream.c
    )
    target_include_directories(opusfile PUBLIC opusfile/include)
    target_link_libraries(opusfile PUBLIC ogg opus)
    target_compile_definitions(opusfile PUBLIC SCXT_USE_OPUS=1)

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        # third party warnings are somebody else's problem
        target_compile_options(opusfile PRIVATE -w)
    endif()
else ()
    add_library(opusfile INTERFACE)
endif ()