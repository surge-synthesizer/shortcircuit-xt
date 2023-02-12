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

#include "sinc_table.h"
#include <cmath>

namespace scxt::dsp
{

inline double sincf(double x)
{
    if (x == 0)
        return 1;
    return (sin(M_PI * x)) / (M_PI * x);
}

inline double BESSI0(double X)
{
    double Y, P1, P2, P3, P4, P5, P6, P7, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, AX, BX;
    P1 = 1.0;
    P2 = 3.5156229;
    P3 = 3.0899424;
    P4 = 1.2067429;
    P5 = 0.2659732;
    P6 = 0.360768e-1;
    P7 = 0.45813e-2;
    Q1 = 0.39894228;
    Q2 = 0.1328592e-1;
    Q3 = 0.225319e-2;
    Q4 = -0.157565e-2;
    Q5 = 0.916281e-2;
    Q6 = -0.2057706e-1;
    Q7 = 0.2635537e-1;
    Q8 = -0.1647633e-1;
    Q9 = 0.392377e-2;
    if (fabs(X) < 3.75)
    {
        Y = (X / 3.75) * (X / 3.75);
        return (P1 + Y * (P2 + Y * (P3 + Y * (P4 + Y * (P5 + Y * (P6 + Y * P7))))));
    }
    else
    {
        AX = fabs(X);
        Y = 3.75 / AX;
        BX = exp(AX) / sqrt(AX);
        AX = Q1 +
             Y * (Q2 + Y * (Q3 + Y * (Q4 + Y * (Q5 + Y * (Q6 + Y * (Q7 + Y * (Q8 + Y * Q9)))))));
        return (AX * BX);
    }
}

inline double SymmetricKaiser(double x, uint16_t nint, double Alpha)
{
    double N = (double)nint;
    x += N * 0.5;

    if (x > N)
        x = N;
    if (x < 0.0)
        x = 0.0;
    // x = std::min((double)x, (double)N);
    // x = std::max(x, 0.0);
    double a = (2.0 * x / N - 1.0);
    return BESSI0(M_PI * Alpha * sqrt(1.0 - a * a)) / BESSI0(M_PI * Alpha);
}

void SincTable::init()
{
    if (initialized)
        return;

    float cutoff = 0.95f;
    float cutoffI16 = 0.95f;
    for (auto j = 0U; j < FIRipol_M + 1; j++)
    {
        for (auto i = 0U; i < FIRipol_N; i++)
        {
            double t = -double(i) + double(FIRipol_N / 2.0) + double(j) / double(FIRipol_M) - 1.0;
            double val = (float)(SymmetricKaiser(t, FIRipol_N, 5.0) * cutoff * sincf(cutoff * t));

            SincTableF32[j * FIRipol_N + i] = val;
        }
    }
    for (auto j = 0U; j < FIRipol_M; j++)
    {
        for (auto i = 0U; i < FIRipol_N; i++)
        {
            SincOffsetF32[j * FIRipol_N + i] =
                (float)((SincTableF32[(j + 1) * FIRipol_N + i] - SincTableF32[j * FIRipol_N + i]) *
                        (1.0 / 65536.0));
        }
    }

    for (auto j = 0U; j < FIRipol_M + 1; j++)
    {
        for (auto i = 0U; i < FIRipolI16_N; i++)
        {
            double t =
                -double(i) + double(FIRipolI16_N / 2.0) + double(j) / double(FIRipol_M) - 1.0;
            double val =
                (float)(SymmetricKaiser(t, FIRipol_N, 5.0) * cutoffI16 * sincf(cutoffI16 * t));

            SincTableI16[j * FIRipolI16_N + i] = val * 16384;
        }
    }
    for (auto j = 0U; j < FIRipol_M; j++)
    {
        for (auto i = 0U; i < FIRipolI16_N; i++)
        {
            SincOffsetI16[j * FIRipolI16_N + i] =
                (SincTableI16[(j + 1) * FIRipolI16_N + i] - SincTableI16[j * FIRipolI16_N + i]);
        }
    }

    initialized = true;
}
SincTable sincTable;
} // namespace scxt::dsp
