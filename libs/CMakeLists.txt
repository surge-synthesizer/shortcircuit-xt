
add_subdirectory(JUCE)
add_subdirectory(clap-juce-extensions clap-extensions EXCLUDE_FROM_ALL)

add_subdirectory(fmt)
add_subdirectory(taocpp_json)

add_subdirectory(sst/sst-cpputils)
add_subdirectory(sst/sst-plugininfra)
add_subdirectory(sst/sst-filters)
add_subdirectory(sst/sst-jucegui)

# Add a catch2 target
add_library(catch2 INTERFACE)
target_include_directories(catch2 INTERFACE catch2/include)
add_library(shortcircuit::catch2 ALIAS catch2)

# Add a SIMDE target
add_library(simde INTERFACE)
target_include_directories(simde INTERFACE simde)
add_library(shortcircuit::simde ALIAS simde)


# The RWQueue adds install targets so just do this
add_library(readerwriterqueue INTERFACE)
target_include_directories(readerwriterqueue INTERFACE readerwriterqueue)
add_library(shortcircuit::readerwriterqueue ALIAS readerwriterqueue)

add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE miniaudio/include)
add_library(shortcircuit::miniaudio ALIAS miniaudio)

if (APPLE)
    set(RTMIDI_API_JACK FALSE CACHE BOOL "skip jack")
endif()
set(RTMIDI_BUILD_STATIC_LIBS TRUE CACHE BOOL "static rdmidi")
set(RTMIDI_BUILD_TESTING OFF CACHE BOOL "no tests from rtmidi")
add_subdirectory(rtmidi)
