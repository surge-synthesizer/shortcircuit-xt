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

#ifndef SCXT_SRC_SCXT_CORE_SAMPLE_IMPORT_SUPPORT_IMPORT_NUMERIC_H
#define SCXT_SRC_SCXT_CORE_SAMPLE_IMPORT_SUPPORT_IMPORT_NUMERIC_H

#include <cmath>

namespace scxt::import_support
{
// 100 cents per semitone.
inline constexpr float centsToSemitones(float cents) { return cents * 0.01f; }

// Centibels (1/10 dB) to linear amplitude. SF2 sustain levels are encoded this way.
inline float centibelsToLinear(double cB) { return (float)std::pow(10.0, (-cB / 10.0) * 0.05); }

// SF2 pan is -64..63 (signed byte) — negative half divides by 64, positive by 63.
inline float sf2NormalizedPan(int pan) { return pan < 0 ? pan / 64.f : pan / 63.f; }

// Convert dB to the "cubic attenuation" parameter scxt uses on group amplitudes
// (the runtime applies value*value*value to get linear amplitude, after
// clamping value to 0..1). To invert: go to linear amplitude first, then
// cube-root that. Note that group.cpp clamps the cube input to [0, 1] before
// applying, so positive-dB input here will saturate at 0 dB at runtime.
inline float dBToCubicAttenuation(float dB) { return (float)std::cbrt(std::pow(10.0, dB / 20.0)); }
} // namespace scxt::import_support

#endif
