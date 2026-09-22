/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "drop_mapping.h"

#include <algorithm>
#include <cmath>

namespace scxt::engine
{

std::vector<DropRange> dropRangesFor(const DropGeometry &g)
{
    if (g.isMappedInstrument || g.nElements <= 0)
        return {{60, 0, 127}};

    auto n = g.nElements;
    auto rootKey = g.key;

    // the top of the travel is touchy, so give the widest span a band rather than a point
    static constexpr float zoneTrim{0.15f};
    auto fromTop = std::clamp((g.fromTop - zoneTrim) / (1.f - zoneTrim), 0.f, 1.f);
    auto span = (1.0f - std::sqrt(fromTop)) * 80;

    if (n == 1 || g.alt)
    {
        auto low = std::clamp(rootKey - span, 0.f, 127.f);
        auto high = std::clamp(rootKey + span, 0.f, 127.f);

        return {{(int16_t)rootKey, (int16_t)low, (int16_t)high}};
    }

    if (g.shift)
    {
        auto low = std::clamp(rootKey - span, 0.f, 127.f);
        auto high = std::clamp(rootKey + span, 0.f, 127.f);
        float velSpread = 127.0 / n;
        float cVel{0.f};
        int nextS{0};
        if (n >= 127)
            velSpread = 1;

        std::vector<DropRange> ranges;
        for (int i = 0; i < n; ++i)
        {
            auto end = cVel + velSpread;
            int endI = std::min((int)std::round(end), 127);
            if (i == n - 1)
                endI = 127;
            int start = std::min(nextS, endI - 1);
            if (i == 0)
                start = 0;
            nextS = endI + 1;
            cVel = end;
            ranges.emplace_back((int16_t)rootKey, (int16_t)low, (int16_t)high, start, endI);
        }

        return ranges;
    }

    /* OK multi-element case */
    auto per = span / (n - 1);
    auto rPer = std::max((int)std::round(per), 1);
    auto toRoot = (int)(rPer / 2);
    auto nwid = rPer * n;
    auto start = rootKey - nwid / 2;

    // bound constrain so end <= 127 and start >= 0
    if (n < 127)
    {
        if (start < 0)
            start = 0;
        // signed: the spread can be wider than the keyboard, and 127 - nwid then goes negative
        if (start + nwid > 127)
            start = std::max(127 - nwid, 0);
    }
    else
    {
        start = 0;
    }

    std::vector<DropRange> ranges;
    for (int i = 0; i < n; ++i)
    {
        ranges.emplace_back(start + toRoot, start, start + rPer - 1);
        start += rPer;
        if (start + rPer - 1 > 127)
            start = 127 - rPer;
    }
    return ranges;
}

} // namespace scxt::engine
