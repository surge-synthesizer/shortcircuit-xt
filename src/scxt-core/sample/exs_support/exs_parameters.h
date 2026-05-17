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

#ifndef SCXT_SRC_SCXT_CORE_SAMPLE_EXS_SUPPORT_EXS_PARAMETERS_H
#define SCXT_SRC_SCXT_CORE_SAMPLE_EXS_SUPPORT_EXS_PARAMETERS_H

#include <cstdint>

namespace scxt::exs_support
{

// Named parameter IDs for the EXS24 "parameters" block. The raw .exs file
// stores instrument-global settings as (id, value) pairs in a sparse map;
// these are the IDs the Logic Sampler editor uses for things like filter
// cutoff, envelope times, LFO rates, mod-matrix slots, etc.
//
// All identifiers and values below are reverse-engineered, transcribed from:
//
//     ConvertWithMoss by Jürgen Moßgraber
//     https://github.com/git-moss/ConvertWithMoss
//     src/main/java/de/mossgrabers/convertwithmoss/format/exs/
//     EXS24Parameters.java   (LGPL-3.0)
//
// Used here under LGPL-3.0; if/when this file gets meaningfully extended
// please keep the attribution intact. Some IDs only appear in the newer
// (200-parameter) layout but the ID space is shared with the older one.
enum class EXSParam : uint16_t
{
    // Master / global
    MASTER_VOLUME = 0x07,
    MASTER_PAN = 0x160,
    VOLUME_KEYSCALE = 0x08,
    PITCH_BEND_UP = 0x03,
    PITCH_BEND_DOWN = 0x04,
    MONO_LEGATO = 0x0a,
    MIDI_MONO_MODE = 0x116,
    MIDI_MONO_MODE_PITCH_RANGE = 0x11a,
    POLYPHONY_VOICES = 0x05,
    TRANSPOSE = 0x2d,
    COARSE_TUNE = 0x0e,
    FINE_TUNE = 0x0f,
    GLIDE = 0x14,
    PORTA_DOWN = 0x48,
    PORTA_UP = 0x49,

    // Filter 1
    FILTER1_TOGGLE = 0x2c,
    FILTER1_TYPE = 0xf3,
    FILTER1_FAT = 0xaa,
    FILTER1_CUTOFF = 0x1e,
    FILTER1_RESO = 0x1d,
    FILTER1_DRIVE = 0x4b,
    FILTER1_KEYTRACK = 0x2e,

    // Filter 2 (later EXS revisions)
    FILTER2_TOGGLE = 0x174,
    FILTER2_TYPE = 0x186,
    FILTER2_CUTOFF = 0x177,
    FILTER2_RESO = 0x178,
    FILTER2_DRIVE = 0x179,
    FILTERS_SERIAL_PARALLEL = 0x173,
    FILTERS_BLEND = 0x17a,

    // LFO 1
    LFO_1_FADE = 0x3c,
    LFO_1_RATE = 0x3d,
    LFO_1_WAVE_SHAPE = 0x3e,
    LFO_1_KEY_TRIGGER = 0x14c,
    LFO_1_MONO_POLY = 0x14d,
    LFO_1_PHASE = 0x14e,
    LFO_1_POSITIVE_OR_MIDPOINT = 0x14f,
    LFO_1_TEMPO_SYNC = 0x150,
    LFO_1_FADE_IN_OR_OUT = 0x187,

    // LFO 2
    LFO_2_RATE = 0x3f,
    LFO_2_WAVE_SHAPE = 0x40,
    LFO_2_FADE = 0x151,
    LFO_2_KEY_TRIGGER = 0x152,
    LFO_2_MONO_POLY = 0x153,
    LFO_2_PHASE = 0x154,
    LFO_2_POSITIVE_OR_MIDPOINT = 0x155,
    LFO_2_TEMPO_SYNC = 0x156,
    LFO_2_FADE_IN_OR_OUT = 0x188,

    // LFO 3
    LFO_3_RATE = 0xa7,
    LFO_3_WAVE_SHAPE = 0x158,
    LFO_3_FADE = 0x157,
    LFO_3_KEY_TRIGGER = 0x159,
    LFO_3_MONO_POLY = 0x15a,
    LFO_3_PHASE = 0x15b,
    LFO_3_POSITIVE_OR_MIDPOINT = 0x15c,
    LFO_3_TEMPO_SYNC = 0x15d,
    LFO_3_FADE_IN_OR_OUT = 0x189,

    // Envelope 2 (filter env in Logic; 0=AD, 1=AR, 2=ADSR, 3=AHDSR, 4=DADSR, 5=DAHDSR)
    ENV2_TYPE = 0x16a,
    ENV2_VEL_SENS = 0x17d,
    ENV2_DELAY_START = 0x16c,
    ENV2_ATK_CURVE = 0x192,
    ENV2_ATK_HI_VEL = 0x4c,
    ENV2_ATK_LO_VEL = 0x4d,
    ENV2_HOLD = 0x38,
    ENV2_DECAY = 0x4e,
    ENV2_SUSTAIN = 0x4f,
    ENV2_RELEASE = 0x50,
    ENV2_TIME_CURVE = 0x5b,

    // Envelope 1 (amp env in Logic; same type-byte enumeration)
    ENV1_TYPE = 0x6b,
    ENV1_VEL_SENS = 0x5a,
    ENV1_VOLUME_HIGH = 0x59,
    ENV1_DELAY_START = 0x16d,
    ENV1_ATK_CURVE = 0x195,
    ENV1_ATK_HI_VEL = 0x52,
    ENV1_ATK_LO_VEL = 0x53,
    ENV1_HOLD = 0x58,
    ENV1_DECAY = 0x54,
    ENV1_SUSTAIN = 0x51,
    ENV1_RELEASE = 0x55,
    // ENV1_TIME_CURVE has no ID per Java reference; intentionally omitted.

    // Velocity / random / xfade / unison
    AMP_VELOCITY_CURVE = 0x183,
    VELOCITY_OFFSET = 0x5f,
    RANDOM_VELOCITY = 0xa4,
    RANDOM_SAMPLE_SEL = 0xa3,
    RANDOM_PITCH = 0x62,
    XFADE_AMOUNT = 0x61,
    XFADE_TYPE = 0xa5,
    UNISON_TOGGLE = 0xab,
    COARSE_TUNE_REMOTE = 0xa6,
    HOLD_VIA_CONTROL = 0xac,

    // Mod-matrix slots 1..11 (each is 8 contiguous IDs).
    MOD1_DESTINATION = 0xad,
    MOD1_SOURCE = 0xae,
    MOD1_VIA = 0xaf,
    MOD1_AMOUNT_LOW = 0xb0,
    MOD1_AMOUNT_HIGH = 0xb1,
    MOD1_SRC_INVERT = 0xe9,
    MOD1_VIA_INVERT = 0xb2,
    MOD1_BYPASS = 0xf4,

    MOD2_DESTINATION = 0xb3,
    MOD2_SOURCE = 0xb4,
    MOD2_VIA = 0xb5,
    MOD2_AMOUNT_LOW = 0xb6,
    MOD2_AMOUNT_HIGH = 0xb7,
    MOD2_SRC_INVERT = 0xea,
    MOD2_VIA_INVERT = 0xb8,
    MOD2_BYPASS = 0xf5,

    MOD3_DESTINATION = 0xb9,
    MOD3_SOURCE = 0xba,
    MOD3_VIA = 0xbb,
    MOD3_AMOUNT_LOW = 0xbc,
    MOD3_AMOUNT_HIGH = 0xbd,
    MOD3_SRC_INVERT = 0xeb,
    MOD3_VIA_INVERT = 0xbe,
    MOD3_BYPASS = 0xf6,

    MOD4_DESTINATION = 0xbf,
    MOD4_SOURCE = 0xc0,
    MOD4_VIA = 0xc1,
    MOD4_AMOUNT_LOW = 0xc2,
    MOD4_AMOUNT_HIGH = 0xc3,
    MOD4_SRC_INVERT = 0xec,
    MOD4_VIA_INVERT = 0xc4,
    MOD4_BYPASS = 0xf7,

    MOD5_DESTINATION = 0xc5,
    MOD5_SOURCE = 0xc6,
    MOD5_VIA = 0xc7,
    MOD5_AMOUNT_LOW = 0xc8,
    MOD5_AMOUNT_HIGH = 0xc9,
    MOD5_SRC_INVERT = 0xed,
    MOD5_VIA_INVERT = 0xca,
    MOD5_BYPASS = 0xf8,

    MOD6_DESTINATION = 0xcb,
    MOD6_SOURCE = 0xcc,
    MOD6_VIA = 0xcd,
    MOD6_AMOUNT_LOW = 0xce,
    MOD6_AMOUNT_HIGH = 0xcf,
    MOD6_SRC_INVERT = 0xee,
    MOD6_VIA_INVERT = 0xd0,
    MOD6_BYPASS = 0xf9,

    MOD7_DESTINATION = 0xd1,
    MOD7_SOURCE = 0xd2,
    MOD7_VIA = 0xd3,
    MOD7_AMOUNT_LOW = 0xd4,
    MOD7_AMOUNT_HIGH = 0xd5,
    MOD7_SRC_INVERT = 0xef,
    MOD7_VIA_INVERT = 0xd6,
    MOD7_BYPASS = 0xfa,

    MOD8_DESTINATION = 0xd7,
    MOD8_SOURCE = 0xd8,
    MOD8_VIA = 0xd9,
    MOD8_AMOUNT_LOW = 0xda,
    MOD8_AMOUNT_HIGH = 0xdb,
    MOD8_SRC_INVERT = 0xf0,
    MOD8_VIA_INVERT = 0xdc,
    MOD8_BYPASS = 0xfb,

    MOD9_DESTINATION = 0xdd,
    MOD9_SOURCE = 0xde,
    MOD9_VIA = 0xdf,
    MOD9_AMOUNT_LOW = 0xe0,
    MOD9_AMOUNT_HIGH = 0xe1,
    MOD9_SRC_INVERT = 0xf1,
    MOD9_VIA_INVERT = 0xe2,
    MOD9_BYPASS = 0xfc,

    MOD10_DESTINATION = 0xe3,
    MOD10_SOURCE = 0xe4,
    MOD10_VIA = 0xe5,
    MOD10_AMOUNT_LOW = 0xe6,
    MOD10_AMOUNT_HIGH = 0xe7,
    MOD10_SRC_INVERT = 0xf2,
    MOD10_VIA_INVERT = 0xe8,
    MOD10_BYPASS = 0xfd,

    MOD11_DESTINATION = 0x19b,
    MOD11_SOURCE = 0x19c,
    MOD11_VIA = 0x19d,
    MOD11_AMOUNT_LOW = 0x19e,
    MOD11_AMOUNT_HIGH = 0x19f,
    MOD11_SRC_INVERT = 0x1a0,
    MOD11_VIA_INVERT = 0x1a1,
    MOD11_BYPASS = 0x1a2,
};

} // namespace scxt::exs_support

#endif
