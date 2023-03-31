
add_subdirectory(JUCE)
add_subdirectory(clap-juce-extensions clap-extensions EXCLUDE_FROM_ALL)

add_subdirectory(fmt)
add_subdirectory(taocpp_json)

add_subdirectory(xiph)

add_subdirectory(sst/sst-cpputils)
add_subdirectory(sst/sst-plugininfra)
add_subdirectory(sst/sst-filters)
add_subdirectory(sst/sst-jucegui)
add_subdirectory(sst/sst-basic-blocks)
add_subdirectory(sst/libgig-modified)

# Add a catch2 target
add_library(catch2 INTERFACE)
target_include_directories(catch2 INTERFACE catch2/include)
add_library(shortcircuit::catch2 ALIAS catch2)

# Add a SIMDE target
add_library(simde INTERFACE)
target_include_directories(simde INTERFACE simde)
add_library(shortcircuit::simde ALIAS simde)

# OddSound doesn't have a cmake so...
add_library(mts-esp-client STATIC MTS-ESP/Client/libMTSClient.cpp)
target_include_directories(mts-esp-client PUBLIC MTS-ESP/Client)
if (UNIX AND NOT APPLE)
    target_link_libraries(mts-esp-client PUBLIC dl)
endif()
