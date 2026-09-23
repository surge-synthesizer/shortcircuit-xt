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
#include <array>
#include <cmath>

namespace scxt::engine
{
namespace
{
// above 24 semitones the pull climbs in octaves, and the last rung fills the keyboard
constexpr std::array<int16_t, 9> octaveRungs{36, 48, 60, 72, 84, 96, 108, 120, 127};

constexpr float evenPortion{0.7f}; // the first 70% of the pull runs 0..24 evenly
constexpr int16_t evenTopSpan{24};
constexpr float fitToKeyboardBand{0.9f}; // the top 10% pulls the start down to make room

// the pull runs the other way to fromTop: 0 at the bottom of the travel, 1 at the top
float pullOf(float fromTop) { return 1.f - std::clamp(fromTop, 0.f, 1.f); }

int16_t lastKeyOf(int firstKey, int width) { return (int16_t)std::min(firstKey + width - 1, 127); }

// tile 0..127 across the elements, all of them sharing one key range
std::vector<DropRange> velocitySplit(int n, int16_t root, int16_t keyLo, int16_t keyHi, float bend)
{
    std::vector<DropRange> ranges;
    ranges.reserve(n);

    auto gamma = std::pow(2.f, -std::clamp(bend, -1.f, 1.f));
    int start{0};

    for (int i = 0; i < n; ++i)
    {
        int end{127};
        if (i < n - 1)
        {
            auto t = std::pow((float)(i + 1) / n, gamma);
            end = std::clamp((int)std::round(127.f * t), 0, 127);
        }
        // with more samples than velocities the bands run out of room, so stop inverting
        end = std::max(end, start);
        ranges.emplace_back(root, keyLo, keyHi, (int16_t)start, (int16_t)end);
        start = std::min(end + 1, 127);
    }

    return ranges;
}
} // namespace

bool isFitToKeyboardAt(float fromTop) { return pullOf(fromTop) >= fitToKeyboardBand; }

int16_t zoneSpanAt(float fromTop)
{
    auto pull = pullOf(fromTop);

    if (pull <= evenPortion)
        return (int16_t)std::round(pull / evenPortion * evenTopSpan);

    auto t = (pull - evenPortion) / (1.f - evenPortion);
    auto rungs = (int)octaveRungs.size();
    return octaveRungs[std::clamp((int)(t * rungs), 0, rungs - 1)];
}

std::vector<DropRange> dropRangesFor(const DropGeometry &g)
{
    if (g.isMappedInstrument || g.nElements <= 0)
        return {{60, 0, 127}};

    auto n = g.nElements;
    auto key = (int16_t)std::clamp(g.key, 0.f, 127.f);
    auto span = g.overKeyboard ? (int16_t)0 : zoneSpanAt(g.fromTop);
    auto width = span + 1;

    // at the top of the pull the start slides down to make room, which is what gives a
    // single sample the whole keyboard; everywhere else the overflow piles onto the last note
    auto lastStart =
        isFitToKeyboardAt(g.fromTop) && !g.overKeyboard ? std::max(127 - span, 0) : 127;

    auto zoneAt = [&](int i) {
        auto lo = (int16_t)std::clamp(std::min(key + i * width, lastStart), 0, 127);
        auto hi = lastKeyOf(lo, width);
        // one sample roots in the middle of its zone; a spread roots each zone at its
        // lower edge, or under the cursor where the start slid away from it
        auto root =
            n == 1 ? (int16_t)((lo + hi) / 2) : (int16_t)std::clamp((int)key, (int)lo, (int)hi);
        return DropRange(root, lo, hi);
    };

    auto first = zoneAt(0);

    if (g.alt)
        return {first};

    // the upper half of the keyboard splits over velocity without needing shift
    if (g.shift || (g.overKeyboard && !g.inLowerKeyboardHalf))
        return velocitySplit(n, first.root, first.keyLo, first.keyHi, g.velocityBend);

    if (g.overKeyboard)
        return std::vector<DropRange>(n, first);

    std::vector<DropRange> ranges;
    ranges.reserve(n);
    for (int i = 0; i < n; ++i)
        ranges.push_back(zoneAt(i));
    return ranges;
}

} // namespace scxt::engine
