/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#ifndef SCXT_SRC_ENGINE_MACROS_H
#define SCXT_SRC_ENGINE_MACROS_H

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <algorithm>

#include "sst/basic-blocks/params/ParamMetadata.h"

namespace scxt::engine
{
struct Macro
{
    float value{0.f}; // -1 .. 1

    bool isBipolar{false}; // The value is expected in range -1..1
    bool isStepped{false}; // The value has discrete stepped value
    size_t stepCount{1};   // how many steps between min and max.
                           // So 1 means a 0..1 or -1..1 toggle
    std::string name{};

    void setIsBipolar(bool b)
    {
        isBipolar = b;
        setValueConstrained(value);
    }
    void setValueConstrained(float f) { value = std::clamp(f, isBipolar ? -1.f : 0.f, 1.f); }
    void setFrom01(float f)
    {
        assert(f >= 0.f && f <= 1.f);
        if (isBipolar)
        {
            value = f * 2 - 1.0;
        }
        else
        {
            value = f;
        }
    }
};
} // namespace scxt::engine
#endif // SHORTCIRCUITXT_MACROS_H
