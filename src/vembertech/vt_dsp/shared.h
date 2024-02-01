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
#ifndef SCXT_SRC_VEMBERTECH_VT_DSP_SHARED_H
#define SCXT_SRC_VEMBERTECH_VT_DSP_SHARED_H

#if defined(__aarch64__)
#define SIMDE_ENABLE_NATIVE_ALIASES
#include "simde/x86/sse2.h"
#else
#include <xmmintrin.h>
#include <emmintrin.h>

#if LINUX
#include <immintrin.h>
#endif
#endif

inline float i2f_binary_cast(int i)
{
    float *f = (float *)&i;
    return *f;
};

const __m128 m128_mask_signbit = _mm_set1_ps(i2f_binary_cast(0x80000000));
const __m128 m128_mask_absval = _mm_set1_ps(i2f_binary_cast(0x7fffffff));
const __m128 m128_zero = _mm_set1_ps(0.0f);
const __m128 m128_half = _mm_set1_ps(0.5f);
const __m128 m128_one = _mm_set1_ps(1.0f);
const __m128 m128_two = _mm_set1_ps(2.0f);
const __m128 m128_four = _mm_set1_ps(4.0f);
const __m128 m128_1234 = _mm_set_ps(1.f, 2.f, 3.f, 4.f);
const __m128 m128_0123 = _mm_set_ps(0.f, 1.f, 2.f, 3.f);

#endif // SCXT_SRC_VEMBERTECH_VT_DSP_SHARED_H
