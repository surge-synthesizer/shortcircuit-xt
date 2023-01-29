//
// Created by Paul Walker on 2/1/23.
//

#include "equal.h"
#include <cmath>
#include <algorithm>

namespace scxt::tuning
{
void EqualTuningProvider::init()
{
    for (int i = 0; i < tuning_table_size; i++)
    {
        table_pitch[i] = powf(2.f, ((float)i - 256.f) * (1.f / 12.f));
        table_pitch_inv[i] = 1.f / table_pitch[i];
    }

    for (int i = 0; i < 1001; ++i)
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
    int e = (int)x;
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