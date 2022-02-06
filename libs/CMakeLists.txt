
if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    add_subdirectory(pybind11)
endif ()
add_subdirectory(JUCE)
add_subdirectory(fmt)

add_subdirectory(sst/sst-cpputils)
add_subdirectory(sst/sst-plugininfra)

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

