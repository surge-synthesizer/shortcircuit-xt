/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#ifndef SCXT_SRC_MODULATION_MODULATORS_ENVLFO_H
#define SCXT_SRC_MODULATION_MODULATORS_ENVLFO_H

#include "utils.h"

#include "sst/basic-blocks/modulators/DAHDSREnvelope.h"

namespace scxt::modulation::modulators
{
struct EnvLFO : SampleRateSupport
{
    using env_t = sst::basic_blocks::modulators::DAHDSREnvelope<EnvLFO, blockSize>;

    float envelope_rate_linear_nowrap(float f)
    {
        return blockSize * sampleRateInv * dsp::twoToTheXTable.twoToThe(-f);
    }

    env_t envelope{this};
    float output{0.f};
    void attack(float delay) { envelope.attack(delay); }

    void process(float delay, float attack, float hold, float decay, float sustain, float release,
                 bool isGated)
    {
        envelope.processBlock01AD(delay, attack, hold, decay, sustain, release, isGated);
        output = envelope.outBlock0;
    }
};
};     // namespace scxt::modulation::modulators
#endif // SHORTCIRCUITXT_ENVLFO_H
