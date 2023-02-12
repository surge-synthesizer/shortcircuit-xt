/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include "globals.h"
#include "synthesis/mathtables.h"
#include <assert.h>

float note_to_pitch(float x)
{
    assert(x > -256.f);
    assert(x < 256.f);

    /*x += 256;
    int e = (int)x;
    float a = x - (float)e;

    return (1.f-a)*table_pitch[e&0x1ff] + a*table_pitch[(e+1)&0x1ff];*/

    const __m128 one = _mm_set1_ps(1.f);
    __m128 f = _mm_load_ss(&x);
    int e = _mm_cvtss_si32(f);
    __m128 a = _mm_sub_ss(f, _mm_cvtsi32_ss(a, e));
    e += 256;

    _mm_store_ss(&x,
                 _mm_add_ss(_mm_mul_ss(_mm_sub_ss(one, a), _mm_load_ss(&table_pitch[e & 0x1ff])),
                            _mm_mul_ss(a, _mm_load_ss(&table_pitch[(e + 1) & 0x1ff]))));
    return x;
}

float db_to_linear(float x)
{
    /*x += 384;
    int e = (int)x;
    float a = x - (float)e;

    return (1.f-a)*table_dB[e&0x1ff] + a*table_dB[(e+1)&0x1ff];*/

    const __m128 one = _mm_set1_ps(1.f);
    __m128 f = _mm_load_ss(&x);
    int e = _mm_cvtss_si32(f);
    __m128 a = _mm_sub_ss(f, _mm_cvtsi32_ss(a, e));
    e += 384;

    _mm_store_ss(&x, _mm_add_ss(_mm_mul_ss(_mm_sub_ss(one, a), _mm_load_ss(&table_dB[e & 0x1ff])),
                                _mm_mul_ss(a, _mm_load_ss(&table_dB[(e + 1) & 0x1ff]))));
    return x;
}

float tanh_turbo(float x)
{
    x *= 32.f;
    x += 512.f;
    int e = (int)x;
    float a = x - (float)e;

    if (e > 0x3fd)
        return 1;
    if (e < 1)
        return -1;

    return (1 - a) * waveshapers[0][e & 0x3ff] + a * waveshapers[0][(e + 1) & 0x3ff];
}

float envelope_rate_linear(float x)
{
    x *= 16.f;
    x += 256.f;
    int e = (int)x;
    float a = x - (float)e;

    return (1.f - a) * table_envrate_linear[e & 0x1ff] + a * table_envrate_linear[(e + 1) & 0x1ff];
}
