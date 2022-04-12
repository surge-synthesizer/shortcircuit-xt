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

#pragma once

#include <cmath>
#include <math.h>
#define _USE_MATH_DEFINES

extern float table_dB[512], table_pitch[512], table_envrate_lpf[512], table_envrate_linear[512];
extern float waveshapers[8][1024]; // typ?

inline double shafted_tanh(double x) { return (exp(x) - exp(-x * 1.2)) / (exp(x) + exp(-x)); }

inline void init_tables(double samplerate, int block_size)
{
    float db60 = powf(10.f, 0.05f * -60.f);
    for (int i = 0; i < 512; i++)
    {
        table_dB[i] = powf(10.f, 0.05f * ((float)i - 384.f));
        table_pitch[i] = powf(2.f, ((float)i - 256.f) * (1.f / 12.f));
        double k = samplerate * pow(2.0, (((double)i - 256.0) / 16.0)) / (double)block_size;
        table_envrate_lpf[i] = (float)(1.f - exp(log(db60) / k));
        table_envrate_linear[i] = 1 / k;
    }

    double mult = 1.0 / 32.0;
    for (int i = 0; i < 1024; i++)
    {
        double x = ((double)i - 512.0) * mult;
        waveshapers[0][i] = (float)tanh(x);                              // wst_tanh,
        waveshapers[1][i] = (float)powf(tanh(powf(fabs(x), 5.f)), 0.2f); // wst_hard
        if (x < 0)
            waveshapers[1][i] = -waveshapers[1][i];
        waveshapers[2][i] = (float)shafted_tanh(x + 0.5) - shafted_tanh(0.5); // wst_assym
        waveshapers[3][i] =
            (float)sin((double)((double)i - 512.0) * 3.14159265 / 512.0);    // wst_sinus
        waveshapers[4][i] = (float)tanh((double)((double)i - 512.0) * mult); // wst_digi
    }

    //	nyquist_pitch = 12.f*log(M_PI / ((1/samplerate) * 2*M_PI*440.0))/log(2.0);
}

float note_to_pitch(float x);
float db_to_linear(float x);
float tanh_turbo(float x);
float envelope_rate_linear(float x);