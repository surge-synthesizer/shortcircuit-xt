
# Add a SIMDE target
add_library(simde INTERFACE)
target_include_directories(simde INTERFACE simde)
add_library(shortcircuit::simde ALIAS simde)

add_subdirectory(clap/clap)
add_subdirectory(clap/clap-helpers)

set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES TRUE CACHE BOOL "Get em")
set(CLAP_WRAPPER_DONT_ADD_TARGETS TRUE CACHE BOOL "I'll targetize")
set(CLAP_WRAPPER_BUILD_AUV2 TRUE CACHE BOOL "It's only logical")
if (APPLE)
    # BaconPaul has jack kinda installed
    set(RTAUDIO_API_JACK FALSE CACHE BOOL "Not on apple")
endif()
add_subdirectory(clap/clap-wrapper)

add_subdirectory(sst/sst-clap-helpers)
if (NOT DEFINED JUCE_PATH)
    set(JUCE_PATH "${CMAKE_SOURCE_DIR}/libs/JUCE")
endif()
add_clap_juce_shim(JUCE_PATH ${JUCE_PATH})


add_subdirectory(fmt)
# Modify this basedon the outcome of https://github.com/taocpp/PEGTL/issues/347
set(PEGTL_NO_STD_FILESYSTEM ON CACHE BOOL "Skip PEGTL FileSystem")
add_subdirectory(taocpp_json)
add_subdirectory(sqlite3-3.42.0)

add_subdirectory(xiph)

add_subdirectory(eurorack)

add_subdirectory(sst/sst-cpputils)

# set(SST_PLUGININFRA_FILESYSTEM_FORCE_PLATFORM ON CACHE BOOL "Force platform filesystem")
set(SST_PLUGININFRA_PROVIDE_TINYXML ON CACHE BOOL "Get TinyXML")  # need this for UserDefaults
set(SST_PLUGININFRA_PROVIDE_MINIZ ON CACHE BOOL "Get MiniZ")  # need this for UserDefaults
add_subdirectory(sst/sst-basic-blocks)
add_subdirectory(sst/sst-plugininfra)
add_subdirectory(sst/sst-filters)
add_subdirectory(sst/sst-waveshapers)
add_subdirectory(sst/sst-effects)
add_subdirectory(sst/sst-jucegui)
add_subdirectory(sst/sst-voicemanager)
add_subdirectory(sst/libgig-modified)

# Add a catch2 target
add_library(catch2 INTERFACE)
target_include_directories(catch2 INTERFACE catch2/include)
add_library(shortcircuit::catch2 ALIAS catch2)


# OddSound doesn't have a cmake so...
add_library(mts-esp-client STATIC MTS-ESP/Client/libMTSClient.cpp)
target_include_directories(mts-esp-client PUBLIC MTS-ESP/Client)
if (UNIX AND NOT APPLE)
    target_link_libraries(mts-esp-client PUBLIC dl)
endif ()

add_library(minimp3 INTERFACE)
if (${SCXT_USE_MP3})
    message(STATUS "Including MP3 support with minimp3")
    # minimpe doesn't have a cmake but is header only
    target_include_directories(minimp3 INTERFACE minimp3)
    target_compile_definitions(minimp3 INTERFACE SCXT_USE_MP3=1)
endif()

add_subdirectory (melatonin_inspector)
add_subdirectory(md5sum)