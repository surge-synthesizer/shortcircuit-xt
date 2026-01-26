/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_CORE_MODULATION_HAS_MODULATORS_H
#define SCXT_SRC_SCXT_CORE_MODULATION_HAS_MODULATORS_H

#include "configuration.h"

#include "sst/basic-blocks/modulators/AHDSRShapedSC.h"
#include "modulation/modulators/steplfo.h"
#include "modulation/modulators/curvelfo.h"
#include "modulation/modulators/envlfo.h"
#include "modulation/modulators/random_evaluator.h"
#include "modulation/modulators/phasor_evaluator.h"
#include "sst/cpputils/constructors.h"

namespace scxt::modulation::shared
{

static_assert(lfosPerGroup == lfosPerZone,
              "If this is false you need to template out the count below");

template <typename T, size_t egsPerObject> struct HasModulators
{
    struct DoubleRate
    {
        double sampleRate{1}, sampleRateInv{1};
        T *that{nullptr};
        explicit DoubleRate(T *that) : that(that) { resetRates(); }
        void resetRates()
        {
            sampleRate = that->sampleRate * 2;
            sampleRateInv = that->sampleRateInv / 2;
        }
    } doubleRate;

    using cLFO_t = scxt::modulation::modulators::CurveLFO;
    HasModulators(T *that, sst::basic_blocks::dsp::RNG &extrng)
        : eg{sst::cpputils::make_array<ahdsrenv_t, egsPerObject>(that)}, doubleRate{that},
          egOS{sst::cpputils::make_array<ahdsrenvOS_t, egsPerObject>(&doubleRate)},
          randomEvaluator(extrng),
          curveLfos{sst::cpputils::make_array<cLFO_t, lfosPerObject>(extrng)}
    {
    }

    static constexpr uint16_t lfosPerObject{lfosPerZone};
    static_assert(egsPerObject != 0);

    enum LFOEvaluator
    {
        STEP,
        CURVE,
        ENV,
        MSEG
    } lfoEvaluator[lfosPerObject]{};
    std::array<scxt::modulation::modulators::StepLFO, lfosPerObject> stepLfos{};
    std::array<scxt::modulation::modulators::CurveLFO, lfosPerObject> curveLfos;
    std::array<scxt::modulation::modulators::EnvLFO, lfosPerObject> envLfos{};
    scxt::modulation::modulators::RandomEvaluator randomEvaluator;
    scxt::modulation::modulators::PhasorEvaluator phasorEvaluator{};

    typedef sst::basic_blocks::modulators::AHDSRShapedSC<
        T, blockSize, sst::basic_blocks::modulators::TwentyFiveSecondExp>
        ahdsrenv_t;

    typedef sst::basic_blocks::modulators::AHDSRShapedSC<
        DoubleRate, (blockSize << 1), sst::basic_blocks::modulators::TwentyFiveSecondExp>
        ahdsrenvOS_t;

    std::array<ahdsrenv_t, egsPerObject> eg;
    std::array<ahdsrenvOS_t, egsPerObject> egOS;

    std::array<bool, lfosPerObject> lfosActive{};
    std::array<bool, egsPerObject> egsActive{};
    bool phasorsActive{false};

    void setHasModulatorsSampleRate(double sr, double sri) { doubleRate.resetRates(); }

    std::array<bool, egsPerObject> doEGRetrigger{};
    std::array<float, egsPerObject> lastEGRetrigger{};
    std::array<bool, lfosPerObject> doLFORetrigger{};
    std::array<float, lfosPerObject> lastLFORetrigger{};
    template <typename Endpoints> void updateRetriggers(Endpoints *endpoints)
    {
        for (int i = 0; i < egsPerObject; ++i)
        {
            auto nrt = *(endpoints->egTarget[i].retriggerP);
            doEGRetrigger[i] = false;
            if (lastEGRetrigger[i] < 0.5 && nrt > 0.5)
            {
                doEGRetrigger[i] = true;
            }
            lastEGRetrigger[i] = nrt;
        }
        for (int i = 0; i < lfosPerObject; ++i)
        {
            auto nrt = *(endpoints->lfo[i].retriggerP);
            doLFORetrigger[i] = false;
            if (lastLFORetrigger[i] < 0.5 && nrt > 0.5)
            {
                doLFORetrigger[i] = true;
            }
            lastLFORetrigger[i] = nrt;
        }
    }

    bool getEnvSpecificGate(bool keyGate, const modulation::modulators::AdsrStorage &adsr,
                            ahdsrenv_t::Stage stage)
    {
        switch (adsr.gateMode)
        {
        case modulation::modulators::AdsrStorage::GateMode::GATED:
            return keyGate;
        case modulation::modulators::AdsrStorage::GateMode::SKIP_SUSTAIN:
            return keyGate && stage < ahdsrenv_t::s_sustain;
        case modulation::modulators::AdsrStorage::GateMode::ONESHOT:
            return stage < ahdsrenv_t::s_sustain;
        }
        return keyGate;
    }
};
} // namespace scxt::modulation::shared
#endif // SHORTCIRCUITXT_HAS_MODULATORS_H
