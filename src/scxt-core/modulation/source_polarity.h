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

#ifndef SCXT_SRC_SCXT_CORE_MODULATION_SOURCE_POLARITY_H
#define SCXT_SRC_SCXT_CORE_MODULATION_SOURCE_POLARITY_H

#include <cstdint>

namespace scxt::modulation
{
// the sign of what a source emits, which decides whether a depth reads one way or both
enum struct SourcePolarity : int32_t
{
    UNIPOLAR = 0, // 0 .. 1
    BIPOLAR = 1,  // -1 .. 1
    NEGATIVE = 2  // -1 .. 0
};

// the polarity of source * via
constexpr SourcePolarity productPolarity(SourcePolarity a, SourcePolarity b)
{
    if (a == SourcePolarity::BIPOLAR || b == SourcePolarity::BIPOLAR)
        return SourcePolarity::BIPOLAR;
    return (a == SourcePolarity::NEGATIVE) != (b == SourcePolarity::NEGATIVE)
               ? SourcePolarity::NEGATIVE
               : SourcePolarity::UNIPOLAR;
}

// what a curve makes of each input polarity; the default leaves it alone
struct CurvePolarity
{
    SourcePolarity fromUnipolar{SourcePolarity::UNIPOLAR};
    SourcePolarity fromBipolar{SourcePolarity::BIPOLAR};
    SourcePolarity fromNegative{SourcePolarity::NEGATIVE};

    static constexpr CurvePolarity always(SourcePolarity p) { return {p, p, p}; }

    constexpr SourcePolarity apply(SourcePolarity p) const
    {
        switch (p)
        {
        case SourcePolarity::BIPOLAR:
            return fromBipolar;
        case SourcePolarity::NEGATIVE:
            return fromNegative;
        case SourcePolarity::UNIPOLAR:
            break;
        }
        return fromUnipolar;
    }
};
} // namespace scxt::modulation

#endif // SCXT_SRC_SCXT_CORE_MODULATION_SOURCE_POLARITY_H
