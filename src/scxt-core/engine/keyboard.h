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
    /*
     * Fades in short circuit work relative to the range and are interior regions.
     * So a keyboard region with start/end at 48/60 with a fadeStart of 4 and fadeEnd
     * of 2 will have note 48 have velocity 0, then 1/4, 1/2, 3/4 up to 1 and note 52
     *
     * This leads to a few constraints
     * fadestart > 0
     * fadeEnd > 0
     * fadeStart + fadeEnd < start - end + 1
     */
    int16_t fadeStart{0}, fadeEnd{0};

    KeyboardRange() = default;
    KeyboardRange(int s, int e) : keyStart(s), keyEnd(e) { normalize(); }

    void normalize()
    {
        if (keyEnd < keyStart)
            std::swap(keyStart, keyEnd);

        if (fadeStart + fadeEnd > keyEnd - keyStart + 1)
        {
            // TODO: Handle this case - but I thikn the semantic of the test is wrong
            SCLOG("Potentially erroneous fade points in keyboard: "
                  << SCD(fadeStart) << SCD(fadeEnd) << SCD(keyStart) << SCD(keyEnd));
        }
    }

    float fadeAmpltiudeAt(int16_t toKey)
    {
        if (toKey < keyStart + fadeStart && fadeStart > 0)
        {
            return std::clamp(static_cast<float>(toKey - keyStart) / fadeStart, 0.f, 1.f);
        }
        else if (toKey > keyEnd - fadeEnd && fadeEnd > 0)
        {
            return std::clamp(static_cast<float>(keyEnd - toKey) / fadeEnd, 0.f, 1.f);
        }

        return 1;
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
        return (keyStart == other.keyStart && keyEnd == other.keyEnd &&
                fadeStart == other.fadeStart && fadeEnd == other.fadeEnd);
    }
    bool operator!=(const KeyboardRange &other) const { return !(*this == other); }
};

struct VelocityRange
{
    int16_t velStart{0}, velEnd{127}; // Start is >= end is <= so a single note has start == end

    // See comment above
    int16_t fadeStart{0}, fadeEnd{0};

    VelocityRange() = default;
    VelocityRange(int s, int e) : velStart(s), velEnd(e) { normalize(); }

    void normalize()
    {
        if (velEnd < velStart)
            std::swap(velStart, velEnd);

        if (fadeStart + fadeEnd > velEnd - velStart)
        {
            // TODO: Handle this case - but I think the semantic of the test is wrong
            SCLOG("Potentialy erroneous fade points in velocity: " << SCD(fadeStart) << SCD(fadeEnd)
                                                                   << SCD(velStart) << SCD(velEnd));
        }
    }

    bool includes(int16_t vel)
    {
        if (vel < 0)
            return false;
        if (vel >= velStart && vel <= velEnd)
            return true;
        return false;
    }

    float fadeAmpltiudeAt(int16_t toVel)
    {
        if (toVel < velStart + fadeStart && fadeStart > 0)
        {
            return std::clamp(static_cast<float>(toVel - velStart) / fadeStart, 0.f, 1.f);
        }
        else if (toVel > velEnd - fadeEnd && fadeEnd > 0)
        {
            return std::clamp(static_cast<float>(velEnd - toVel) / fadeEnd, 0.f, 1.f);
        }
        return 1;
    }

    bool operator==(const VelocityRange &other) const
    {
        return (velStart == other.velStart && velEnd == other.velEnd &&
                fadeStart == other.fadeStart && fadeEnd == other.fadeEnd);
    }
    bool operator!=(const VelocityRange &other) const { return !(*this == other); }
};
} // namespace scxt::engine
#endif // __SCXT_ENGINE_KEYBOARD_H
