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
#include "bitcrusher.h"

#include "datamodel/parameter.h"
#include "tuning/equal.h"
#include "engine/memory_pool.h"
#include "dsp/data_tables.h"

#include <algorithm>

namespace scxt::dsp::processor::distortion
{
BitCrusher::BitCrusher(engine::MemoryPool *mp, float *f, int32_t *i)
    : Processor(proct_fx_bitcrusher, mp, f, i), lp(this)
{
    parameter_count = 5;
    ctrlmode_desc[0] = datamodel::pmd().withName("samplerate");
    ctrlmode_desc[1] = datamodel::pmd().asPercent().withName("bitdepth");
    ctrlmode_desc[2] = datamodel::pmd().asPercent().withName("zeropoint");

    // LP2B params
    ctrlmode_desc[3] = datamodel::pitchInOctavesFromA440().withName("cutoff");
    ctrlmode_desc[4] = datamodel::pmd().asPercent().withName("resonance");

    dsamplerate_inv = samplerate_inv;
}

BitCrusher::~BitCrusher() {}

void BitCrusher::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = 7.5f;
    param[1] = 1.0f;
    param[2] = 0.0f;
    param[3] = 5.5f;
    param[4] = 0.0f;
}

void BitCrusher::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                                float pitch)
{
    float t = samplerate_inv * 440 * tuning::equalTuning.note_to_pitch(12 * param[0]);
    float bd = 16.f * std::min(1.f, std::max(0.f, param[1]));
    float b = powf(2, bd), b_inv = 1.f / b;

    int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        float inputL = datainL[k];
        float inputR = datainR[k];
        time[0] -= t;
        time[1] -= t;
        if (time[0] < 0.f)
        {
            float val = inputL * b;
            val += param[2];
            level[0] = (float)((int)val) * b_inv;
            time[0] += 1.0f;
            time[0] = std::max(time[0], 0.f);
        }
        if (time[1] < 0.f)
        {
            float val = inputR * b;
            val += param[2];
            level[1] = (float)((int)val) * b_inv;
            time[1] += 1.0f;
            time[1] = std::max(time[1], 0.f);
        }
        dataoutL[k] = level[0];
        dataoutR[k] = level[1];
    }

    lp.coeff_LP2B(lp.calc_omega(param[3]), lp.calc_v1_Q(param[4]));
    lp.process_block(dataoutL, dataoutR);
}

float BitCrusher::note_to_pitch_ignoring_tuning(float n)
{
    return tuning::equalTuning.note_to_pitch(n);
}
float BitCrusher::dbToLinear(float n) { return dbTable.dbToLinear(n); }
} // namespace scxt::dsp::processor::distortion