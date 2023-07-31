/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include <stdexcept>
#include "base_matrix.h"

namespace scxt::modulation
{
std::string getModMatrixCurveStreamingName(const ModMatrixCurve &dest)
{
    switch (dest)
    {
    case modc_none:
        return "modc_none";
    case modc_cube:
        return "modc_cube";

    case numModMatrixCurves:
        throw std::logic_error("Don't call with numModMatrixCurves");
    }

    throw std::logic_error("Mysterious unhandled condition");
}
std::optional<ModMatrixCurve> fromModMatrixCurveStreamingName(const std::string &s)
{
    for (int i = modc_none; i < numModMatrixCurves; ++i)
    {
        if (getModMatrixCurveStreamingName((ModMatrixCurve)i) == s)
        {
            return ((ModMatrixCurve)i);
        }
    }
    return modc_none;
}

std::string getModMatrixCurveDisplayName(const ModMatrixCurve &dest)
{
    switch (dest)
    {
    case modc_none:
        return "";
    case modc_cube:
        return "Cube";

    case numModMatrixCurves:
        throw std::logic_error("Don't call with numModMatrixCurves");
    }

    throw std::logic_error("Mysterious unhandled condition");
}

modMatrixCurveNames_t getModMatrixCurveNames()
{
    modMatrixCurveNames_t res;
    for (int s = (int)modc_none; s < numModMatrixCurves; ++s)
    {
        auto vms = (ModMatrixCurve)s;
        res.emplace_back(vms, getModMatrixCurveDisplayName(vms));
    }
    return res;
}
} // namespace scxt::modulation