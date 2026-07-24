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

#ifndef SCXT_SRC_SCXT_CORE_MODULATION_HAS_MODULATORS_H
#define SCXT_SRC_SCXT_CORE_MODULATION_HAS_MODULATORS_H

#include "configuration.h"

#include <cmath>

#include "sst/basic-blocks/modulators/AHDSRShapedSC.h"
#include "modulation/modulators/steplfo.h"
#include "modulation/modulators/curvelfo.h"
#include "modulation/modulators/envlfo.h"
#include "modulation/modulators/random_evaluator.h"
#include "modulation/modulators/phasor_evaluator.h"
#include "modulators/env_follower.h"
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
    using envF_t = scxt::modulation::modulators::EnvFollower;
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
    std::array<scxt::modulation::modulators::EnvFollower, envFollowersPerGroupOrZone>
        envelopeFollowers{};

    typedef sst::basic_blocks::modulators::AHDSRShapedSC<
        T, blockSize, sst::basic_blocks::modulators::TwentyFiveSecondExp, true>
        ahdsrenv_t;

    typedef sst::basic_blocks::modulators::AHDSRShapedSC<
        DoubleRate, (blockSize << 1), sst::basic_blocks::modulators::TwentyFiveSecondExp, true>
        ahdsrenvOS_t;

    std::array<ahdsrenv_t, egsPerObject> eg;
    std::array<ahdsrenvOS_t, egsPerObject> egOS;

    std::array<bool, lfosPerObject> lfosActive{};
    std::array<bool, egsPerObject> egsActive{};
    std::array<bool, scxt::envFollowersPerGroupOrZone> envFollowersActive{};
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

    /*
     * Per-LFO trigger bookkeeping. RELEASE arms at attack and fires on the gate's
     * falling edge; everything else starts immediately.
     */
    struct LFOTriggerState
    {
        bool started{false};
        bool armed{false};
    };
    std::array<LFOTriggerState, lfosPerObject> lfoTrigger{};

    /*
     * The phase (0..1) a curve LFO should start at for this trigger mode. SONGPOS locks
     * to song position at attack and free-runs after, so a note at bar 3 lands where the
     * cycle would have been had it been running since the song started.
     */
    static float initialLFOPhase(const modulation::ModulatorStorage &ms,
                                 const sst::basic_blocks::modulators::Transport &transport,
                                 float rate, sst::basic_blocks::dsp::RNG &rng)
    {
        switch (ms.triggerMode)
        {
        case modulation::ModulatorStorage::RANDOM:
            return rng.unif01();
        case modulation::ModulatorStorage::SONGPOS:
        {
            // Same table SimpleLFO's own phase increment goes through, so the locked
            // phase and the free-run frequency agree exactly rather than to table error
            auto effRate = modulation::modulators::CurveLFO::snappedRate(ms, rate);
            auto freqHz = (double)dsp::twoToTheXTable.twoToThe(effRate) *
                          (ms.temposync ? transport.tempo / 120.0 : 1.0);
            auto ph = ms.start_phase + transport.timeInSeconds * freqHz;
            return (float)(ph - std::floor(ph));
        }
        default:
            return ms.start_phase;
        }
    }

    /*
     * The step sequencer needs its position expressed as (step, phase within step)
     * rather than a bare phase, so it gets its own entry point.
     */
    void startStepLFO(int i, const modulation::ModulatorStorage &ms,
                      const sst::basic_blocks::modulators::Transport &transport,
                      sst::basic_blocks::dsp::RNG &rng)
    {
        switch (ms.triggerMode)
        {
        case modulation::ModulatorStorage::SONGPOS:
            stepLfos[i].setStartPosition(ms.start_phase, transport.timeInSeconds);
            break;
        case modulation::ModulatorStorage::RANDOM:
            stepLfos[i].setStartPosition(rng.unif01());
            break;
        default:
            stepLfos[i].setStartPosition(ms.start_phase);
            break;
        }
    }

    /*
     * Fire LFO i: position it for its trigger mode and mark it running. The evaluator
     * must already have been assigned and had its sample rate set.
     */
    template <typename LFOEndpoint>
    void startLFO(int i, const modulation::ModulatorStorage &ms,
                  const sst::basic_blocks::modulators::Transport &transport,
                  sst::basic_blocks::dsp::RNG &rng, const LFOEndpoint &lp)
    {
        lfoTrigger[i] = {true, false};
        switch (lfoEvaluator[i])
        {
        case STEP:
            startStepLFO(i, ms, transport, rng);
            break;
        case CURVE:
            curveLfos[i].attack(initialLFOPhase(ms, transport, *lp.rateP, rng), *lp.curve.delayP,
                                ms.modulatorShape);
            break;
        case ENV:
            envLfos[i].attack(*lp.env.delayP, *lp.env.attackP);
            break;
        default:
            break;
        }
    }

    void silenceLFO(int i)
    {
        switch (lfoEvaluator[i])
        {
        case STEP:
            stepLfos[i].silence();
            break;
        case CURVE:
            curveLfos[i].silence();
            break;
        case ENV:
            envLfos[i].silence();
            break;
        default:
            break;
        }
    }

    /*
     * The gate an LFO's own envelope (curve DAR / env DAHDSR) should see. ONESHOT drops
     * the gate once the envelope is past its peak so it makes exactly one pass; RELEASE
     * holds the gate open from the moment it fires, since the key is already up.
     */
    template <typename Stage>
    static bool lfoEnvelopeGate(modulation::ModulatorStorage::TriggerMode tm, bool keyGate,
                                Stage stage, Stage sustainStage)
    {
        switch (tm)
        {
        case modulation::ModulatorStorage::RELEASE:
            return true;
        case modulation::ModulatorStorage::ONESHOT:
            return stage < sustainStage;
        default:
            return keyGate;
        }
    }

    /*
     * One LFO's per-block work: honor the trigger mode (deferred start, release-fire),
     * then run whichever evaluator this LFO is and scale by amplitude. Shared verbatim
     * by Voice (gate = isGated) and Group (gate = gated) so the two can't drift.
     */
    template <typename LFOEndpoint>
    void processLFOBlock(int i, const modulation::ModulatorStorage &ms, bool gate,
                         const sst::basic_blocks::modulators::Transport &transport,
                         sst::basic_blocks::dsp::RNG &rng, const LFOEndpoint &lp)
    {
        bool rt = doLFORetrigger[i];
        doLFORetrigger[i] = false;

        if (!lfoTrigger[i].started)
        {
            // RELEASE fires on the gate falling edge; an explicit retrigger fires it too
            if (!rt && !(lfoTrigger[i].armed && !gate))
            {
                silenceLFO(i);
                return;
            }
            startLFO(i, ms, transport, rng, lp);
            rt = false;
        }

        auto oneShot = (ms.triggerMode == modulation::ModulatorStorage::ONESHOT);
        auto amp = *(lp.amplitudeP);

        switch (lfoEvaluator[i])
        {
        case STEP:
            if (rt)
                stepLfos[i].retrigger();
            stepLfos[i].process(blockSize);
            stepLfos[i].output *= amp;
            break;
        case CURVE:
        {
            if (rt)
                curveLfos[i].attack(0, *lp.curve.delayP, ms.modulatorShape);

            // ONESHOT is the DAR envelope running exactly once, so it forces the env on
            using senv_t = modulation::modulators::CurveLFO::senv_t;
            auto useGate =
                lfoEnvelopeGate(ms.triggerMode, gate, curveLfos[i].simpleEnv.stage, senv_t::s_hold);

            curveLfos[i].process(*lp.rateP, *lp.curve.deformP, *lp.curve.angleP, *lp.curve.delayP,
                                 *lp.curve.attackP, *lp.curve.releaseP,
                                 ms.curveLfoStorage.useenv || oneShot, ms.curveLfoStorage.unipolar,
                                 useGate);
            curveLfos[i].output *= amp;
            break;
        }
        case ENV:
        {
            using env_t = modulation::modulators::EnvLFO::env_t;
            auto eloop = ms.envLfoStorage.loop && !oneShot;
            auto useGate =
                lfoEnvelopeGate(ms.triggerMode, gate, envLfos[i].envelope.stage, env_t::s_sustain);

            if (eloop)
            {
                // A looping env is a unipolar LFO: gate through DAHD, drop at sustain so it
                // releases, then re-attack once it completes so it cycles. This drives the
                // loop for both Voice and Group (the Group path previously lacked it).
                useGate = envLfos[i].envelope.stage < env_t::s_sustain;
                if (envLfos[i].envelope.stage > env_t::s_release)
                    rt = true;
            }
            if (rt)
            {
                envLfos[i].attackFrom(envLfos[i].output, *lp.env.delayP, *lp.env.attackP);
                useGate = true;
            }

            envLfos[i].process(*lp.env.delayP, *lp.env.attackP, *lp.env.holdP, *lp.env.decayP,
                               *lp.env.sustainP, *lp.env.releaseP, *lp.env.aShapeP, *lp.env.dShapeP,
                               *lp.env.rShapeP, *lp.env.rateMulP, useGate, ms.temposync,
                               transport.tempo / 120.f);
            envLfos[i].output *= amp;
            break;
        }
        default:
            break;
        }
    }

    static bool evaluateGate(bool keyGate, modulation::modulators::AdsrStorage::GateMode mode,
                             ahdsrenv_t::Stage stage, bool samplePlaying = false)
    {
        switch (mode)
        {
        case modulation::modulators::AdsrStorage::GateMode::GATED:
            return keyGate;
        case modulation::modulators::AdsrStorage::GateMode::SEMI_GATED:
            return keyGate && stage < ahdsrenv_t::s_sustain;
        case modulation::modulators::AdsrStorage::GateMode::ONESHOT:
            return stage < ahdsrenv_t::s_sustain;
        case modulation::modulators::AdsrStorage::GateMode::SAMPLE_GATED:
            return samplePlaying;
        }
        return keyGate;
    }

    bool getEnvSpecificGate(bool keyGate, const modulation::modulators::AdsrStorage &adsr,
                            ahdsrenv_t::Stage stage, bool samplePlaying = false)
    {
        return evaluateGate(keyGate, adsr.gateMode, stage, samplePlaying);
    }
};
} // namespace scxt::modulation::shared
#endif // SHORTCIRCUITXT_HAS_MODULATORS_H
