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
#ifndef SCXT_SRC_ENGINE_KEYBOARD_H
#define SCXT_SRC_ENGINE_KEYBOARD_H

#include <utility>
#include <cassert>

namespace scxt::engine
{
/**
 * A class representing a range of the keyboard with fade zones on either
 * end. The range covers keyStart to keyEnd with a note length fade in
 * on either side.
 */
struct KeyboardRange
{
    int16_t keyStart{-1}, keyEnd{-1}; // Start is >= end is <= so a single note has start == end
    int16_t fadeStart{0}, fadeEnd{0};

    KeyboardRange() = default;
    KeyboardRange(int s, int e) : keyStart(s), keyEnd(e) { normalize(); }

    void normalize()
    {
        if (keyEnd < keyStart)
            std::swap(keyStart, keyEnd);
        if (fadeStart + fadeEnd > keyEnd - keyStart)
        {
            // TODO: Handle this case
            assert(false);
        }
    }

    bool includes(int16_t key)
    {
        if (key < 0)
            return false;
        if (key >= keyStart && key <= keyEnd)
            return true;
        return false;
    }

    bool operator==(const KeyboardRange &other) const
    {
        return (keyStart == other.keyStart &&
                keyEnd == other.keyEnd &&
                fadeStart == other.fadeStart &&
                fadeEnd == other.fadeEnd);
    }
    bool operator!=(const KeyboardRange &other) const
    {
        return ! (*this == other);
    }
};
} // namespace scxt::engine
#endif // __SCXT_ENGINE_KEYBOARD_H
