add_subdirectory(pybind11)
add_subdirectory(JUCE)

add_subdirectory(sst/sst-cpputils)

# Add a catch2 target
add_library(catch2 INTERFACE)
target_include_directories(catch2 INTERFACE catch2/include)
add_library(shortcircuit::catch2 ALIAS catch2)

# Add a SIMDE target
add_library(simde INTERFACE)
target_include_directories(simde INTERFACE simde)
add_library(shortcircuit::simde ALIAS simde)

# Add a filesystem implementation if we need it
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
    #include <filesystem>
    namespace fs = std::filesystem;
    int main(int, char **argv) {
      return fs::recursive_directory_iterator{fs::path{*argv}}.depth();
    }" SC3_FILESYSTEM_STD_FS_FOUND)
add_library(filesystem INTERFACE)
if (SC3_FILESYSTEM_STD_FS_FOUND)
    message(STATUS "Using OS level Filesystem")
else ()
    message(STATUS "Using GHC Filesystem")
    add_subdirectory(ghc-filesystem)
    target_link_libraries(filesystem INTERFACE ghc_filesystem)
    target_compile_definitions(filesystem INTERFACE SC3_USE_GHC_FILESYSTEM=1)
endif ()
add_library(shortcircuit::filesystem ALIAS filesystem)

# Add a catch2 target