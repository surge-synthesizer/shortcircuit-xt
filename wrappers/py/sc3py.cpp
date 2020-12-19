#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <thread>
#include <utility>

#include "filesystem/import.h"
#include "sampler.h"
#include "version.h"

namespace py = pybind11;

int createSC3(float sampleRate) { return (int)sampleRate; }

PYBIND11_MODULE(shortcircuit3py, m)
{
    m.doc() = "Python bindings for ShortCircuit3";
    m.def("createSC3", &createSC3, "Create an SC3 instance", py::arg("sampleRate"));
    m.def(
        "getVersion", []() { return ShortCircuit::Build::FullVersionStr; },
        "Get the version of ShortCircuit");
}