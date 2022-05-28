/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include <pybind11/pybind11.h>

#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <string>
#include <utility>

#include "sampler.h"
#include "version.h"

namespace py = pybind11;

struct SC3PY : public sampler
{
    explicit SC3PY(int numOuts) : sampler(nullptr, numOuts) {}

    void loadFileFullKeyboard(const std::string &s) { load_file(string_to_path(s)); }

    void playNotePy(int ch, int note, int vel) { PlayNote(ch, note, vel); }
    void releaseNotePy(int ch, int note, int vel) { ReleaseNote(ch, note, vel); }

    py::array_t<float> getOutput()
    {
        return py::array_t<float>({{2, (long)BLOCK_SIZE}}, {2 * sizeof(float), sizeof(float)},
                                  (const float *)(&output[0][0]));
    }
};

std::unique_ptr<SC3PY> createSC3(float sampleRate, int nChannels)
{
    auto res = std::make_unique<SC3PY>(2);
    res->set_samplerate(sampleRate);
    return res;
}

PYBIND11_MODULE(shortcircuit3py, m)
{
    m.doc() = "Python bindings for Shortcircuit XT";
    m.def("createSC3", &createSC3, "Create a Shortcircuit XT instance", py::arg("sampleRate"),
          py::arg("nChannels") = py::int_(2));
    m.def(
        "getVersion", []() { return scxt::build::FullVersionStr; },
        "Get the version of Shortcircuit XT");

    py::class_<SC3PY>(m, "ShortcircuitXTSampler")
        .def("loadFileSimple", &SC3PY::loadFileFullKeyboard)

        .def("process_audio", &SC3PY::process_audio)
        .def("getOutput", &SC3PY::getOutput)

        .def("playNote", &SC3PY::playNotePy)
        .def("releaseNote", &SC3PY::releaseNotePy);
}