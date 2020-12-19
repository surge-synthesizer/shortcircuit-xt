#include <pybind11/pybind11.h>

#include <string>
#include <utility>

#include "sampler.h"
#include "version.h"

// This is obviously gross
void *hInstance = nullptr;

namespace py = pybind11;

struct SC3PY : public sampler
{
    explicit SC3PY(int numOuts) : sampler(nullptr, numOuts) {}
};

std::unique_ptr<SC3PY> createSC3(float sampleRate, int nChannels)
{
    auto res = std::make_unique<SC3PY>(2);
    res->set_samplerate(sampleRate);
    return res;
}

PYBIND11_MODULE(shortcircuit3py, m)
{
    m.doc() = "Python bindings for ShortCircuit3";
    m.def("createSC3", &createSC3, "Create an SC3 instance", py::arg("sampleRate"),
          py::arg("nChannels") = py::int_(2));
    m.def(
        "getVersion", []() { return ShortCircuit::Build::FullVersionStr; },
        "Get the version of ShortCircuit");

    py::class_<SC3PY>(m, "ShortCircuit3Sampler");
}