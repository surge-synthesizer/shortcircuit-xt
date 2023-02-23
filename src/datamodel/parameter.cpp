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

#include "parameter.h"
#include <cmath>
#include <cassert>
#include <fmt/core.h>

namespace scxt::datamodel
{
std::string ControlDescription::valueToString(float value) const
{
    if (type == INT)
    {
        // TODO obviouly
        assert(false);
        return "ERROR";
    }
    if (type == FLOAT)
    {
        auto mv = mapScale * value + mapBase;
        switch (displayMode)
        {
        case LINEAR:
            return fmt::format("{:8.4} {}", mv, unit);
        case TWO_TO_THE_X:
            // TODO improve these of course
            return fmt::format("{:8.4} {}", std::pow(2.0, mv), unit);
        default:
            return fmt::format("ERROR {} {} {} {}", __FILE__, __LINE__, mv, unit);
        }
    }

    assert(false);
    return "ERROR";
}

std::optional<float> ControlDescription::stringToValue(const std::string &s) const { return {}; }
} // namespace scxt::datamodel