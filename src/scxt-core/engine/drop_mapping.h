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
#ifndef SCXT_SRC_SCXT_CORE_ENGINE_DROP_MAPPING_H
#define SCXT_SRC_SCXT_CORE_ENGINE_DROP_MAPPING_H

#include <cstdint>
#include <vector>

namespace scxt::engine
{
/**
 * Where one dropped sample lands: its root note and the keyboard and velocity
 * span of the zone it creates.
 */
struct DropRange
{
    DropRange(int16_t r, int16_t kl, int16_t kh)
        : root(r), keyLo(kl), keyHi(kh), velLo(0), velHi(127)
    {
    }
    DropRange(int16_t r, int16_t kl, int16_t kh, int16_t vl, int16_t vh)
        : root(r), keyLo(kl), keyHi(kh), velLo(vl), velHi(vh)
    {
    }
    int16_t root, keyLo, keyHi, velLo, velHi;
};

/**
 * The drop gesture, in units the mapping editor has already resolved from
 * pixels, so the layout the drop produces can be reasoned about without a UI.
 */
struct DropGeometry
{
    int nElements{1};

    // fractional midi note under the cursor, already clamped to the keyboard
    float key{60.f};

    // 0 at the top of the mapping area, 1 at the bottom
    float fromTop{0.f};

    bool shift{false}; // spread over velocity rather than the keyboard
    bool alt{false};   // collapse to one range, to be stacked as variants

    // an sfz or similar carries its own mapping, so the gesture is ignored
    bool isMappedInstrument{false};
};

/**
 * One range per dropped element, in drop order. Always at least one entry, and
 * exactly nElements of them except when the gesture collapses them to a single
 * shared range.
 */
std::vector<DropRange> dropRangesFor(const DropGeometry &);

} // namespace scxt::engine

#endif
