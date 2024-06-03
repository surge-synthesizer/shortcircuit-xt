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

#ifndef SCXT_SRC_MODULATION_MODULATORS_CURVELFO_H
#define SCXT_SRC_MODULATION_MODULATORS_CURVELFO_H

#include "sst/basic-blocks/modulators/SimpleLFO.h"
#include "sst/basic-blocks/modulators/DAREnvelope.h"

#include "utils.h"
#include "modulation/modulator_storage.h"

namespace scxt::modulation::modulators
{
struct CurveLFO : SampleRateSupport
{
    float output{0.f};

    using slfo_t = sst::basic_blocks::modulators::SimpleLFO<CurveLFO, scxt::blockSize>;
    using senv_t = sst::basic_blocks::modulators::DAREnvelope<
        CurveLFO, scxt::blockSize, sst::basic_blocks::modulators::TwentyFiveSecondExp>;
    slfo_t simpleLfo{this};
    senv_t simpleEnv{this};
    friend slfo_t;

    slfo_t::Shape curveShape{slfo_t::SINE};

    float envelope_rate_linear_nowrap(float f)
    {
        return blockSize * sampleRateInv * dsp::twoToTheXTable.twoToThe(-f);
    }

    void attack(float initPhase, ModulatorStorage::ModulatorShape shape)
    {
        switch (shape)
        {
        case ModulatorStorage::LFO_RAMP:
            curveShape = slfo_t::RAMP;
            break;
        case ModulatorStorage::LFO_TRI:
            curveShape = slfo_t::TRI;
            break;
        case ModulatorStorage::LFO_PULSE:
            curveShape = slfo_t::PULSE;
            break;
        case ModulatorStorage::LFO_SMOOTH_NOISE:
            curveShape = slfo_t::SMOOTH_NOISE;
            break;
        case ModulatorStorage::LFO_SH_NOISE:
            curveShape = slfo_t::SH_NOISE;
            break;
        case ModulatorStorage::LFO_SINE:
        default:
            curveShape = slfo_t ::SINE;
            break;
        }

        simpleLfo.attack(curveShape);
        simpleLfo.applyPhaseOffset(initPhase);
        simpleEnv.attack(0.4);
    }

    uint32_t smp{0};
    void process(const float rate, const float deform, const float delay, const float attack,
                 const float release, bool useEnv, bool unipolar, bool isGated)
    {
        simpleLfo.process_block(rate, deform, curveShape);
        auto lfov = simpleLfo.lastTarget;
        if (unipolar)
            lfov = (lfov + 1) * 0.5;
        auto ev = 1.f;
        if (useEnv)
        {
            simpleEnv.processBlock01AD(delay, attack, release, isGated);
            ev = simpleEnv.outBlock0;
        }
        output = lfov * ev;
    }
};
} // namespace scxt::modulation::modulators
#endif // SHORTCIRCUITXT_CURVELFO_H
