/*
 * ShortCircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortCircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortCircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortCircuitXT is inspired by, and shares some code with, the
 * commercial product ShortCircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for ShortCircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortCircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "equal.h"
#include <cmath>
#include <algorithm>

namespace scxt::tuning
{
void EqualTuningProvider::init()
{
    for (auto i = 0U; i < tuning_table_size; i++)
    {
        table_pitch[i] = powf(2.f, ((float)i - 256.f) * (1.f / 12.f));
        table_pitch_inv[i] = 1.f / table_pitch[i];
    }

    for (auto i = 0U; i < 1001; ++i)
    {
        double twelths = i * 1.0 / 12.0 / 1000.0;
        table_two_to_the[i] = pow(2.0, twelths);
        table_two_to_the_minus[i] = pow(2.0, -twelths);
    }
}

float EqualTuningProvider::note_to_pitch(float note)
{
    auto x = std::clamp(note + 256, 1.e-4f, tuning_table_size - (float)1.e-4);
    // x += 256;
    auto e = (int16_t)x;
    float a = x - (float)e;

    float pow2pos = a * 1000.0;
    int pow2idx = (int)pow2pos;
    float pow2frac = pow2pos - pow2idx;
    float pow2v =
        (1 - pow2frac) * table_two_to_the[pow2idx] + pow2frac * table_two_to_the[pow2idx + 1];
    return table_pitch[e] * pow2v;
}

EqualTuningProvider equalTuning;
} // namespace scxt::tuning