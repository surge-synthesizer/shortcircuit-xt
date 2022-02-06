add_subdirectory(headless)
add_subdirectory(juce)

if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    add_subdirectory(py)
endif ()