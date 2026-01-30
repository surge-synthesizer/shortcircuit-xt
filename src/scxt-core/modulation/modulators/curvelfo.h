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

#ifndef SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_CURVELFO_H
#define SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_CURVELFO_H

#include "sst/basic-blocks/modulators/SimpleLFO.h"
#include "sst/basic-blocks/modulators/DAREnvelope.h"
#include "sst/basic-blocks/modulators/Transport.h"
#include "sst/basic-blocks/dsp/RNG.h"

#include "utils.h"
#include "modulation/modulator_storage.h"

namespace scxt::modulation::modulators
{
struct CurveLFO : SampleRateSupport
{
    float output{0.f};

    explicit CurveLFO(sst::basic_blocks::dsp::RNG &extrng) : simpleLfo(this, extrng) {}

    // only use this no-arg for the UI drawing!
    CurveLFO() : simpleLfo(this) { forDisplay = true; }

    using slfo_t = sst::basic_blocks::modulators::SimpleLFO<CurveLFO, scxt::blockSize>;
    using senv_t = sst::basic_blocks::modulators::DAREnvelope<
        CurveLFO, scxt::blockSize, sst::basic_blocks::modulators::TwentyFiveSecondExp>;
    slfo_t simpleLfo;
    senv_t simpleEnv{this};
    friend slfo_t;
    bool forDisplay{false};

    slfo_t::Shape curveShape{slfo_t::SINE};

  private:
    modulation::ModulatorStorage *settings{nullptr};
    sst::basic_blocks::modulators::Transport *td{nullptr};

  public:
    void assign(modulation::ModulatorStorage *s, sst::basic_blocks::modulators::Transport *t)
    {
        settings = s;
        td = t;
    }

    float envelope_rate_linear_nowrap(float f)
    {
        return blockSize * sampleRateInv * dsp::twoToTheXTable.twoToThe(-f);
    }

    void attack(float initPhase, float delay, ModulatorStorage::ModulatorShape shape)
    {
        assert(settings && td);
        switch (shape)
        {
        case ModulatorStorage::LFO_RAMP:
            curveShape = slfo_t::RAMP;
            break;
        case ModulatorStorage::LFO_TRI:
            curveShape = slfo_t::TRI;
            break;
        case ModulatorStorage::LFO_SAW_TRI_RAMP:
            curveShape = slfo_t::SAW_TRI_RAMP;
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

        if (forDisplay)
        {
            simpleLfo.attackForDisplay(curveShape);
        }
        else
        {
            simpleLfo.attack(curveShape);
        }
        simpleLfo.applyPhaseOffset(initPhase);
        simpleEnv.attack(delay);
    }

    uint32_t smp{0};
    datamodel::pmd tsConverter{datamodel::pmd().asLfoRate()};
    void process(const float rate, const float deform, const float angle, const float delay,
                 const float attack, const float release, bool useEnv, bool unipolar, bool isGated)
    {
        float tsRatio{1.0};
        float rt = rate;
        if (settings && td && settings->temposync)
        {
            rt = -tsConverter.snapToTemposync(-rt);
            tsRatio = td->tempo / 120.0;
        }
        simpleLfo.process_block(rt, deform, curveShape, false, tsRatio, angle);
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
