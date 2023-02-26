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
#include "engine/memory_pool.h"
#include "configuration.h"
#include "sst/basic-blocks/mechanics/block-ops.h"
#include "tuning/equal.h"
#include "dsp/data_tables.h"

namespace scxt::dsp::processor
{

namespace mech = sst::basic_blocks::mechanics;

MicroGate::MicroGate(engine::MemoryPool *mp, float *f, int32_t *i)
    : Processor(proct_fx_microgate, mp, f, i)
{
    memoryPool->preReservePool(microgateBlockSize);
    parameter_count = 4;

    // TODO What are the actual units here for HOLD?
    setStr(ctrllabel[0], "hold");
    ctrlmode_desc[0] = datamodel::ControlDescription{datamodel::ControlDescription::FLOAT,
                                                     datamodel::ControlDescription::LINEAR,
                                                     -10,
                                                     0.1,
                                                     10,
                                                     -3,
                                                     "tbd"};
    setStr(ctrllabel[1], "loop");
    ctrlmode_desc[1] = datamodel::cdPercent;
    setStr(ctrllabel[2], "threshold");
    ctrlmode_desc[2] = datamodel::cdDecibel;
    setStr(ctrllabel[3], "reduction");
    ctrlmode_desc[3] = datamodel::cdDecibel;
}

void MicroGate::init()
{
    assert(memoryPool);
    auto data = memoryPool->checkoutBlock(microgateBlockSize);
    memset(data, 0, microgateBlockSize);
    loopbuffer[0] = (float *)data;
    loopbuffer[1] = (float *)(data + microgateBufferSize * sizeof(float));
}

void MicroGate::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = -3.0f;
    param[1] = 0.5f;
    param[2] = -12.0f;
    param[3] = -96.0f;
}

void MicroGate::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                               float pitch)
{
    mech::copy_from_to<blockSize>(datainL, dataoutL);
    mech::copy_from_to<blockSize>(datainR, dataoutR);

    // TODO fixme
    float threshold = dbTable.dbToLinear(param[2]);
    float reduction = dbTable.dbToLinear(param[3]);
    int ihtime = (int)(float)(samplerate * tuning::equalTuning.note_to_pitch(12 * param[0]));

    for (int k = 0; k < BLOCK_SIZE; k++)
    {
        float input = std::max(fabs(datainL[k]), fabs(datainR[k]));

        if ((input > threshold) && !gate_state)
        {
            holdtime = ihtime;
            gate_state = true;
            is_recording[0] = true;
            bufpos[0] = 0;
            is_recording[1] = true;
            bufpos[1] = 0;
        }

        if (holdtime < 0)
            gate_state = false;

        if ((!(onesampledelay[0] * datainL[k] > 0)) && (datainL[k] > 0))
        {
            gate_zc_sync[0] = gate_state;
            int looplimit = (int)(float)(4 + 3900 * param[1] * param[1]);
            if (bufpos[0] > looplimit)
            {
                is_recording[0] = false;
                buflength[0] = bufpos[0];
            }
        }
        if ((!(onesampledelay[1] * datainR[k] > 0)) && (datainR[k] > 0))
        {
            gate_zc_sync[1] = gate_state;
            int looplimit = (int)(float)(4 + 3900 * param[1] * param[1]);
            if (bufpos[1] > looplimit)
            {
                is_recording[1] = false;
                buflength[1] = bufpos[1];
            }
        }
        onesampledelay[0] = datainL[k];
        onesampledelay[1] = datainR[k];

        if (gate_zc_sync[0])
        {
            if (is_recording[0])
            {
                loopbuffer[0][bufpos[0]++ & (microgateBufferSize - 1)] = datainL[k];
            }
            else
            {
                dataoutL[k] = loopbuffer[0][bufpos[0] & (microgateBufferSize - 1)];
                bufpos[0]++;
                if (bufpos[0] >= buflength[0])
                    bufpos[0] = 0;
            }
        }
        else
            dataoutL[k] *= reduction;

        if (gate_zc_sync[1])
        {
            if (is_recording[1])
            {
                loopbuffer[1][bufpos[1]++ & (microgateBufferSize - 1)] = datainR[k];
            }
            else
            {
                dataoutR[k] = loopbuffer[1][bufpos[1] & (microgateBufferSize - 1)];
                bufpos[1]++;
                if (bufpos[1] >= buflength[1])
                    bufpos[1] = 0;
            }
        }
        else
            dataoutR[k] *= reduction;

        holdtime--;
    }
}

MicroGate::~MicroGate()
{
    if (loopbuffer[0])
    {
        memoryPool->returnBlock((engine::MemoryPool::data_t *)loopbuffer[0], microgateBlockSize);
    }
}
} // namespace scxt::dsp::processor