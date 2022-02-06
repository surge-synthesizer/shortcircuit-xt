
# Build the python bindings
if (SCXT_SKIP_PYTHON)
    message(STATUS "Skipping Python Build")
    add_custom_target(shortcircuit3py)
else ()
    message(STATUS "Building Shortcircuit XT Python bindings with pybind11")
    pybind11_add_module(shortcircuit3py)
    target_sources(shortcircuit3py PRIVATE sc3py.cpp)
    target_link_libraries(shortcircuit3py PRIVATE shortcircuit-core)
endif ()