/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_ENVLFO_H
#define SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_ENVLFO_H

#include "utils.h"

#include "sst/basic-blocks/modulators/DAHDSREnvelope.h"

namespace scxt::modulation::modulators
{
struct EnvLFO : SampleRateSupport
{
    using env_t = sst::basic_blocks::modulators::AHDSRShapedSC<
        EnvLFO, blockSize, sst::basic_blocks::modulators::TwentyFiveSecondExp>;

    float envelope_rate_linear_nowrap(float f)
    {
        return blockSize * sampleRateInv * dsp::twoToTheXTable.twoToThe(-f);
    }

    env_t envelope{this};
    float output{0.f};
    void attack(float delay, float attack) { envelope.attackFromWithDelay(0.f, delay, attack); }
    void attackFrom(float level, float delay, float attack)
    {
        envelope.attackFromWithDelay(level, delay, attack);
    }

    void process(float delay, float attack, float hold, float decay, float sustain, float release,
                 float aShape, float dShape, float rShape, float rateMul, bool isGated)
    {
        auto rm = scxt::dsp::twoToTheXTable.twoToThe(rateMul);
        envelope.processBlockWithDelayAndRateMul(delay, attack, hold, decay, sustain, release,
                                                 aShape, dShape, rShape, rm, isGated, false);
        output = envelope.outBlock0;
    }

    float timeTakenBy(float stage) { return envelope.timeInSecondsFromParam(stage); }
    float timeTakenBy(float delay, float attack, float hold, float decay, float release)
    {
        return timeTakenBy(delay) + timeTakenBy(attack) + timeTakenBy(hold) + timeTakenBy(decay) +
               timeTakenBy(release);
    }
};
} // namespace scxt::modulation::modulators
#endif // SHORTCIRCUITXT_ENVLFO_H
