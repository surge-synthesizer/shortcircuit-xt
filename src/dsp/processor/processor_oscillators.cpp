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

#include "processor_defs.h"
#include "dsp/resampling.h"
#include "configuration.h"
#include "dsp/data_tables.h"
#include <cmath>
#include <algorithm>
#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/FastMath.h"
#include "tuning/equal.h"

namespace scxt::dsp::processor
{
namespace mech = sst::basic_blocks::mechanics;

static constexpr int64_t large = 0x10000000000;
static constexpr float integrator_hpf = 0.99999999f;

OscPulseSync::OscPulseSync(engine::MemoryPool *mp, float *fp, int32_t *ip)
    : Processor(ProcessorType::proct_osc_pulse_sync, mp, fp, ip)
{
    parameter_count = 3;

    setStr(ctrllabel[0], "tune");
    ctrlmode_desc[0] = cdMPitch;

    setStr(ctrllabel[1], "width");
    ctrlmode_desc[1] = cdPercentDef;

    setStr(ctrllabel[2], "sync");
    ctrlmode_desc[2] = scxt::datamodel::ControlDescription{datamodel::ControlDescription::FLOAT,
                                                           datamodel::ControlDescription::LINEAR,
                                                           0,
                                                           0.05,
                                                           96,
                                                           0,
                                                           "semitones"};

    first_run = true;
    oscstate = 0;
    syncstate = 0;
    osc_out = 0;
    polarity = false;
    bufpos = 0;
    pitch = 0;

    for (float &v : oscbuffer)
        v = 0.f;
}

void OscPulseSync::init_params()
{
    if (!param)
        return;
    param[0] = 0.0f;
    param[1] = 0.0f;
    param[2] = 2.0f;
}

void OscPulseSync::convolute()
{
    int32_t ipos = (int32_t)(((large + oscstate) >> 16) & 0xFFFFFFFF);
    bool sync = false;
    if (syncstate < oscstate)
    {
        ipos = (int32_t)(((large + syncstate) >> 16) & 0xFFFFFFFF);
        double t = std::max(
            0.5, samplerate / (440.0 * std::pow((double)1.05946309435, (double)pitch + param[0])));
        int64_t syncrate = (int64_t)(double)(65536.0 * 16777216.0 * t);
        oscstate = syncstate;
        syncstate += syncrate;
        sync = true;
    }
    // generate pulse
    float fpol = polarity ? -1.0f : 1.0f;
    int32_t m = ((ipos >> 16) & 0xff) * dsp::FIRipol_N;
    float lipol = ((float)((uint32_t)(ipos & 0xffff)));

    if (!sync || !polarity)
    {
        for (auto k = 0U; k < dsp::FIRipol_N; k++)
        {
            oscbuffer[bufpos + k & (oscillatorBufferLength - 1)] +=
                fpol *
                (dsp::sincTable.SincTableF32[m + k] + lipol * dsp::sincTable.SincOffsetF32[m + k]);
        }
    }

    if (sync)
        polarity = false;

    // add time until next statechange
    double width = (0.5 - 0.499f * std::clamp(param[1], 0.f, 1.f));
    double t = std::max(0.5, samplerate / (440.0 * std::pow((double)1.05946309435,
                                                            (double)pitch + param[0] + param[2])));
    if (polarity)
    {
        width = 1 - width;
    }
    int64_t rate = (int64_t)(double)(65536.0 * 16777216.0 * t * width);

    oscstate += rate;
    polarity = !polarity;
}

void OscPulseSync::process_stereo(float *datain, float *datainR, float *dataout, float *dataoutR,
                                  float pt)
{
    process_mono(datain, dataout, dataoutR, pt);
    mech::copy_from_to<blockSize>(dataout, dataoutR);
}
void OscPulseSync::process_mono(float *datain, float *dataout, float *dataoutR, float pt)
{
    if (first_run)
    {
        first_run = false;

        // initial antipulse
        convolute();
        oscstate -= large;
        int i;
        for (i = 0; i < oscillatorBufferLength; i++)
            oscbuffer[i] *= -0.5f;
        oscstate = 0;
        polarity = 0;
    }
    this->pitch = pt;
    int k;
    for (k = 0; k < blockSize; k++)
    {
        oscstate -= large;
        syncstate -= large;
        while (syncstate < 0)
            this->convolute();
        while (oscstate < 0)
            this->convolute();
        osc_out = osc_out * integrator_hpf + oscbuffer[bufpos];
        dataout[k] = osc_out;
        oscbuffer[bufpos] = 0.f;

        bufpos++;
        bufpos = bufpos & (oscillatorBufferLength - 1);
    }
}

PhaseModulation::PhaseModulation(engine::MemoryPool *mp, float *fp, int32_t *ip)
    : Processor(ProcessorType::proct_osc_phasemod, mp, fp, ip), pre(6, true), post(6, true)
{
    parameter_count = 2;

    setStr(ctrllabel[0], "transpose");
    ctrlmode_desc[0] = cdMPitch;

    setStr(ctrllabel[1], "depth");
    ctrlmode_desc[1] = datamodel::cdDecibel;

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

} // namespace scxt::dsp::processor