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

#include "phasemodulation.h"

#include <algorithm>

#include "datamodel/parameter.h"
#include "oscpulsesync.h"

#include "configuration.h"

#include "infrastructure/sse_include.h"

#include "dsp/resampling.h"
#include "dsp/data_tables.h"

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/FastMath.h"
#include "tuning/equal.h"

namespace scxt::dsp::processor::oscillator
{
namespace mech = sst::basic_blocks::mechanics;

PhaseModulation::PhaseModulation(engine::MemoryPool *mp, float *fp, int32_t *ip)
    : Processor(ProcessorType::proct_osc_phasemod, mp, fp, ip), pre(6, true), post(6, true)
{
    parameter_count = 2;

    ctrlmode_desc[0] = datamodel::pitchTransposition().withName("transpose");
    ctrlmode_desc[1] = datamodel::decibelRange().withName("depth");

    phase = 0;
}
void PhaseModulation::init_params()
{
    if (!param)
        return;

    param[0] = 0.f;
    param[1] = 0.f;
}
void PhaseModulation::process_stereo(float *datainL, float *datainR, float *dataoutL,
                                     float *dataoutR, float pitch)
{
    omega.set_target(0.5 * 440 * tuning::equalTuning.note_to_pitch(pitch + param[0]) * M_PI_2 *
                     samplerate_inv);
    pregain.set_target(3.1415 * dbTable.dbToLinear(param[1]));

    constexpr int bs2 = BLOCK_SIZE << 1;
    float OS alignas(16)[2][bs2];
    float omInterp alignas(16)[bs2];
    float phVals alignas(16)[bs2];

    pregain.multiply_2_blocks_to(datainL, datainR, OS[0], OS[1]);
    pre.process_block_U2(OS[0], OS[1], OS[0], OS[1], bs2);

    omega.store_block(omInterp);
    phVals[0] = phase + omInterp[0];
    for (int k = 1; k < bs2; ++k)
    {
        phVals[k] = phVals[k - 1] + omInterp[k];
    }
    phase = phVals[bs2 - 1];
    if (phase > M_PI)
        phase -= 2 * M_PI;

    namespace bd = sst::basic_blocks::dsp;
    const auto half = _mm_set1_ps(0.5f);
    for (int k = 0; k < bs2; k += 4)
    {
        auto ph = _mm_load_ps(phVals + k);
        auto s0 = _mm_load_ps(OS[0] + k);
        auto s1 = _mm_load_ps(OS[1] + k);
        auto w0 = bd::clampToPiRangeSSE(_mm_add_ps(s0, ph));
        auto w1 = bd::clampToPiRangeSSE(_mm_add_ps(s1, ph));
        ph = bd::clampToPiRangeSSE(ph);
        auto sph = bd::fastsinSSE(ph);

        auto r0 = _mm_mul_ps(half, _mm_sub_ps(bd::fastsinSSE(w0), sph));
        auto r1 = _mm_mul_ps(half, _mm_sub_ps(bd::fastsinSSE(w1), sph));
        _mm_store_ps(OS[0] + k, r0);
        _mm_store_ps(OS[1] + k, r1);
    }

    post.process_block_D2(OS[0], OS[1], bs2, dataoutL, dataoutR);
}

void PhaseModulation::process_mono(float *datain, float *dataoutL, float *dataoutR, float pitch)
{
    omega.set_target(0.5 * 440 * tuning::equalTuning.note_to_pitch(pitch + param[0]) * M_PI_2 *
                     samplerate_inv);
    pregain.set_target(3.1415 * dbTable.dbToLinear(param[1]));

    constexpr int bs2 = BLOCK_SIZE << 1;
    float OS alignas(16)[2][bs2];
    float omInterp alignas(16)[bs2];
    float phVals alignas(16)[bs2];

    pregain.multiply_block_to(datain, OS[0]);
    pre.process_block_U2(OS[0], OS[0], OS[0], OS[1], bs2);

    omega.store_block(omInterp);
    phVals[0] = phase + omInterp[0];
    for (int k = 1; k < bs2; ++k)
    {
        phVals[k] = phVals[k - 1] + omInterp[k];
    }
    phase = phVals[bs2 - 1];
    if (phase > M_PI)
        phase -= 2 * M_PI;

    namespace bd = sst::basic_blocks::dsp;
    const auto half = _mm_set1_ps(0.5f);
    for (int k = 0; k < bs2; k += 4)
    {
        auto ph = _mm_load_ps(phVals + k);
        auto s0 = _mm_load_ps(OS[0] + k);
        auto w0 = bd::clampToPiRangeSSE(_mm_add_ps(s0, ph));
        ph = bd::clampToPiRangeSSE(ph);
        auto sph = bd::fastsinSSE(ph);

        auto r0 = _mm_mul_ps(half, _mm_sub_ps(bd::fastsinSSE(w0), sph));
        _mm_store_ps(OS[0] + k, r0);
    }

    post.process_block_D2(OS[0], OS[0], bs2, dataoutL, 0);
}
} // namespace scxt::dsp::processor::oscillator