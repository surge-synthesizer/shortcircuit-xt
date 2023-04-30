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

#include <cmath>
#include <algorithm>

#include "datamodel/parameter.h"
#include "oscsin.h"

#include "configuration.h"

#include "infrastructure/sse_include.h"

#include "dsp/resampling.h"
#include "dsp/data_tables.h"
#include "dsp/processor/processor_ctrldescriptions.h"

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/FastMath.h"
#include "tuning/equal.h"

namespace scxt::dsp::processor::oscillator
{
namespace mech = sst::basic_blocks::mechanics;

OscSin::OscSin(engine::MemoryPool *mp, float *fp, int32_t *ip)
    : Processor(ProcessorType::proct_osc_sin, mp, fp, ip)
{
    parameter_count = 1;

    ctrlmode_desc[0] = datamodel::pitchTransposition().withName("tune");
}

void OscSin::init_params()
{
    if (!param)
        return;
    param[0] = 0.0f;
}

void OscSin::process_stereo(float *datain, float *datainR, float *dataout, float *dataoutR,
                            float pt)
{
    process_mono(datain, dataout, dataoutR, pt);
    mech::copy_from_to<blockSize>(dataout, dataoutR);
}
void OscSin::process_mono(float *datain, float *dataout, float *dataoutR, float pitch)
{
    quadOsc.setRate(440.0 * 2 * M_PI * tuning::equalTuning.note_to_pitch(param[0] + pitch) *
                    samplerate_inv);

    for (int k = 0; k < BLOCK_SIZE; k++)
    {
        dataout[k] = quadOsc.v;
        quadOsc.step();
    }
}

} // namespace scxt::dsp::processor::oscillator