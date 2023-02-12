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
#include "dsp/sinc_table.h"
#include <cmath>
#include <algorithm>
#include "sst/basic-blocks/mechanics/block-ops.h"

namespace scxt::dsp::processor
{
namespace mech = sst::basic_blocks::mechanics;

static constexpr int64_t large = 0x10000000000;
static constexpr float integrator_hpf = 0.99999999f;

OscPulseSync::OscPulseSync(float *fp, int32_t *ip, bool stereo)
    : Processor(ProcessorType::proct_osc_pulse_sync, fp, ip, stereo)
{
    parameter_count = 3;

    setStr(ctrllabel[0], "tune");
    ctrlmode[0] = datamodel::cm_mod_pitch;
    ctrlmode_desc[0] = cdMPitch;

    setStr(ctrllabel[1], "width");
    ctrlmode[1] = datamodel::cm_percent;
    ctrlmode_desc[1] = cdPercentDef;

    setStr(ctrllabel[2], "sync");
    ctrlmode[2] = datamodel::cm_mod_pitch;
    ctrlmode_desc[2] = scxt::datamodel::ControlDescription{datamodel::ControlDescription::FLOAT, 0, 0.05, 96, 0, "semitones"};

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

void OscPulseSync::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                                  float pitch)
{
    process(0, dataoutL, pitch);
    mech::copy_from_to<blockSize>(dataoutL, dataoutR);
}
void OscPulseSync::process(float *datain, float *dataout, float pitch)
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
    this->pitch = pitch;
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

} // namespace scxt::dsp::processor