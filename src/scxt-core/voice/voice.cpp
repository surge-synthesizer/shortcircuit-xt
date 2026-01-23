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

#include "voice.h"
#include "voice.h"
#include "tuning/equal.h"
#include <cassert>
#include <cmath>

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/PanLaws.h"
#include "engine/engine.h"
#include "dsp/processor/routing.h"

namespace scxt::voice
{

Voice::Voice(engine::Engine *e, engine::Zone *z)
    : scxt::modulation::shared::HasModulators<Voice, egsPerZone>(this, e->rng), engine(e), zone(z),
      sampleIndex(zone->sampleIndex), halfRate(6, true), endpoints(nullptr) // see comment
{
    assert(zone);

    sampleIndexF = sampleIndex;
    if (zone->getNumSampleLoaded() <= 1)
    {
        sampleIndexFraction = 0;
    }
    else
    {
        sampleIndexFraction = sampleIndexF / (zone->getNumSampleLoaded() - 1);
    }

    inLoopF = 0.f;
    loopCountF = 0.f;
    currentLoopPercentageF = 0.f;
    currentSamplePercentageF = 0.f;

    memset(output, 0, 2 * blockSize * sizeof(float));
    memset(processorIntParams, 0, sizeof(processorIntParams));
}

Voice::~Voice()
{
#if BUILD_IS_DEBUG
    if (isVoiceAssigned)
    {
        SCLOG_IF(warnings, "Destroying assigned voice. (OK in shutdown)");
    }
#endif
    for (auto i = 0; i < engine::processorCount; ++i)
    {
        dsp::processor::unspawnProcessor(processors[i]);
        processors[i] = nullptr;
    }
}

void Voice::cleanupVoice()
{
    zone->removeVoice(this);
    zone = nullptr;
    isVoiceAssigned = false;
    engine->voiceManagerResponder.doVoiceEndCallback(this);
    engine->activeVoices--;

    // We cleanup processors here since they may have, say,
    // memory pool resources checked out that others could
    // use which they don't need to hold onto
    for (auto i = 0; i < engine::processorCount; ++i)
    {
        dsp::processor::unspawnProcessor(processors[i]);
        processors[i] = nullptr;
    }
}

void Voice::voiceStarted()
{
    noteExpressions[(int)ExpressionIDs::VOLUME] = 1.0;
    noteExpressions[(int)ExpressionIDs::PAN] = 0.5;
    for (int i = (int)ExpressionIDs::TUNING; i <= (int)ExpressionIDs::PRESSURE; ++i)
    {
        noteExpressions[i] = 0.0;
    }

    forceOversample = zone->parentGroup->outputInfo.oversample;

    lfosActive = zone->lfosActive;
    egsActive = zone->egsActive;
    phasorsActive = zone->phasorsActive;

    for (auto i = 0U; i < engine::lfosPerZone; ++i)
    {
        const auto &ms = zone->modulatorStorage[i];
        lfoEvaluator[i] = ms.isStep() ? STEP : (ms.isEnv() ? ENV : (ms.isMSEG() ? MSEG : CURVE));
    }

    startBeat = engine->transport.timeInBeats;
    updateTransportPhasors();

    // This order matters
    endpoints->sources.bind(modMatrix, *zone, *this);
    modMatrix.prepare(zone->routingTable, getSampleRate(), blockSize);
    endpoints->bindTargetBaseValues(modMatrix, *zone);
    modMatrix.process();

    // These probably need to happen after the modulator is set up
    initializeGenerator();
    initializeProcessors();

    for (auto i = 0U; i < engine::lfosPerZone; ++i)
    {
        if (!lfosActive[i])
        {
            continue;
        }
        const auto &ms = zone->modulatorStorage[i];
        if (lfoEvaluator[i] == STEP)
        {
            stepLfos[i].setSampleRate(sampleRate, sampleRateInv);

            stepLfos[i].assign(&zone->modulatorStorage[i], endpoints->lfo[i].rateP,
                               &engine->transport, engine->rng);
        }
        else if (lfoEvaluator[i] == CURVE)
        {
            curveLfos[i].setSampleRate(sampleRate, sampleRateInv);
            curveLfos[i].assign(&zone->modulatorStorage[i], &engine->transport);
            curveLfos[i].attack(ms.start_phase, *endpoints->lfo[i].curve.delayP, ms.modulatorShape);
        }
        else if (lfoEvaluator[i] == ENV)
        {
            envLfos[i].setSampleRate(sampleRate, sampleRateInv);
            envLfos[i].attack(*endpoints->lfo[i].env.delayP, *endpoints->lfo[i].env.attackP);
        }
        else
        {
            SCLOG_IF(warnings, "Unimplemented modulator shape " << ms.modulatorShape);
        }
    }

    randomEvaluator.evaluate(zone->miscSourceStorage);
    phasorEvaluator.attack(engine->transport, zone->miscSourceStorage, engine->rng);

    auto &aegp = endpoints->egTarget[0];
    if (*aegp.dlyP < 1e-5)
    {
        aeg.attackFrom(0.0, *(aegp.aP) == 0.0); // TODO Envelope Legato Mode
    }
    else
    {
        aeg.attackFromWithDelay(0.0, *aegp.dlyP, *aegp.aP);
    }
    for (int i = 1; i < egsPerZone; ++i)
    {
        if (egsActive[i])
        {
            auto &egp = endpoints->egTarget[i];
            if (*egp.dlyP < 1e-5)
            {
                eg[i].attackFrom(0.0);
            }
            else
            {
                eg[i].attackFromWithDelay(0.0, *egp.dlyP, *egp.aP);
            }
        }
    }
    if (forceOversample)
    {
        if (*aegp.dlyP < 1e-5)
            aegOS.attackFrom(0.0, *(aegp.aP) == 0.0); // TODO Envelope Legato Mode
        else
        {
            aegOS.attackFromWithDelay(0.0, *aegp.dlyP, *aegp.aP);
        }
        // only the AEG needs oversampling since EG2 3 4 is only used at endpoint
    }

    retunedKeyAtAttack =
        zone->getEngine()->midikeyRetuner.retuneRemappedKey(channel, key, originalMidiKey);
    retuneContinuous =
        zone->getEngine()->runtimeConfig.tuningMode != engine::Engine::TuningMode::MTS_NOTE_ON;

    zone->addVoice(this);
}

bool Voice::process()
{
    if (forceOversample)
        return processWithOS<true>();
    else
        return processWithOS<false>();
}

template <bool OS> bool Voice::processWithOS()
{
    namespace mech = sst::basic_blocks::mechanics;

    if (!isVoicePlaying || !isVoiceAssigned || !zone)
    {
        memset(output, 0, sizeof(output));
        return true;
    }

    noteExpressionLags.processAll();
    updateRetriggers(endpoints.get());

    // Run Modulators - these run at base rate never oversampled
    for (auto i = 0; i < engine::lfosPerZone; ++i)
    {
        if (!lfosActive[i])
        {
            continue;
        }
        bool rt = doLFORetrigger[i];
        doLFORetrigger[i] = false;

        if (lfoEvaluator[i] == STEP)
        {
            if (rt)
            {
                stepLfos[i].retrigger();
            }
            stepLfos[i].process(blockSize);
            stepLfos[i].output *= *(endpoints->lfo[i].amplitudeP);
        }
        else if (lfoEvaluator[i] == CURVE)
        {
            auto &lp = endpoints->lfo[i];

            if (rt)
            {
                curveLfos[i].attack(0, *lp.curve.delayP, zone->modulatorStorage[i].modulatorShape);
            }

            curveLfos[i].process(*lp.rateP, *lp.curve.deformP, *lp.curve.angleP, *lp.curve.delayP,
                                 *lp.curve.attackP, *lp.curve.releaseP,
                                 zone->modulatorStorage[i].curveLfoStorage.useenv,
                                 zone->modulatorStorage[i].curveLfoStorage.unipolar, isGated);
            curveLfos[i].output *= *(endpoints->lfo[i].amplitudeP);
        }
        else if (lfoEvaluator[i] == ENV)
        {
            auto &lp = endpoints->lfo[i].env;

            auto eloop = zone->modulatorStorage[i].envLfoStorage.loop;
            auto useGate = isGated;
            if (envLfos[i].envelope.stage > scxt::modulation::modulators::EnvLFO::env_t::s_release)
            {
                rt = true;
            }
            if (eloop)
            {
                useGate = envLfos[i].envelope.stage <
                          scxt::modulation::modulators::EnvLFO::env_t::s_sustain;
            }
            if (rt)
            {
                envLfos[i].attackFrom(envLfos[i].output, *lp.delayP, *lp.attackP);
                useGate = true;
            }

            envLfos[i].process(*lp.delayP, *lp.attackP, *lp.holdP, *lp.decayP, *lp.sustainP,
                               *lp.releaseP, *lp.aShapeP, *lp.dShapeP, *lp.rShapeP, *lp.rateMulP,
                               useGate);
            envLfos[i].output *= *(endpoints->lfo[i].amplitudeP);
        }
        else
        {
        }
    }

    if (phasorsActive)
    {
        phasorEvaluator.step(engine->transport, zone->miscSourceStorage);
    }

    bool envGate{isGated};
    if (sampleIndex >= 0)
    {
        auto &vdata = zone->variantData.variants[sampleIndex];

        envGate = (vdata.playMode == engine::Zone::NORMAL && isGated) ||
                  (vdata.playMode == engine::Zone::ONE_SHOT && isAnyGeneratorRunning) ||
                  (vdata.playMode == engine::Zone::ON_RELEASE && isAnyGeneratorRunning);
    }

    /*
     * Voice always runs the AEG which is default routed, so ignore
     * the egsActive[0] flag
     */
    auto &aegp = endpoints->egTarget[0];
    auto rtaeg = doEGRetrigger[0];
    doEGRetrigger[0] = false;
    if constexpr (OS)
    {
        if (rtaeg)
        {
            aegOS.attackFromWithDelay(aegOS.outBlock0, *aegp.dlyP, *aegp.aP);
        }
        // we need the aegOS for the curve in oversample space
        aegOS.processBlockWithDelay(*aegp.dlyP, *aegp.aP, *aegp.hP, *aegp.dP, *aegp.sP, *aegp.rP,
                                    *aegp.asP, *aegp.dsP, *aegp.rsP, envGate, true);
    }

    // But We need to run the undersample AEG no matter what since it is a modulatino source
    if (rtaeg)
    {
        aeg.attackFromWithDelay(aeg.outBlock0, *aegp.dlyP, *aegp.aP);
    }
    aeg.processBlockWithDelay(*aegp.dlyP, *aegp.aP, *aegp.hP, *aegp.dP, *aegp.sP, *aegp.rP,
                              *aegp.asP, *aegp.dsP, *aegp.rsP, envGate, true);
    // TODO: And output is non zero once we are past attack
    isAEGRunning = (aeg.stage != ahdsrenv_t ::s_complete);

    for (int i = 1; i < egsPerZone; ++i)
    {
        if (egsActive[i])
        {
            if (doEGRetrigger[i])
            {
                doEGRetrigger[i] = false;
                eg[i].attackFromWithDelay(eg[i].outBlock0, *aegp.dlyP, *aegp.aP);
            }

            auto &eg2p = endpoints->egTarget[i];
            eg[i].processBlockWithDelay(*eg2p.dlyP, *eg2p.aP, *eg2p.hP, *eg2p.dP, *eg2p.sP,
                                        *eg2p.rP, *eg2p.asP, *eg2p.dsP, *eg2p.rsP, envGate, false);
        }
    }
    updateTransportPhasors();

    // TODO and probably just want to process the envelopes here
    modMatrix.process();

    if (firstRender)
    {
        /*
         * This is a placeholder for a collection of items which will need doing
         * including checking bounds within loops, dealing with start which
         * pushes past loop end, start which pushes past end, and more. Just a
         * placeholder implementation to get started on that.
         */
        auto [firstIndex, lastIndex] = sampleIndexRange();
        if (firstIndex >= 0 && lastIndex >= 0)
        {
            int currGen{0};
            for (auto currIndex = firstIndex; currIndex < lastIndex; currIndex++)
            {
                auto &s = zone->samplePointers[currIndex];
                auto &variantData = zone->variantData.variants[currIndex];
                if (!variantData.playReverse && s)
                {
                    GD[currGen].samplePos = std::clamp(
                        (int64_t)(GD[currGen].playbackLowerBound +
                                  (*endpoints->sampleTarget.startPosP * s->sampleLengthPerChannel)),
                        (int64_t)0, (int64_t)GD[currGen].playbackUpperBound);
                }
                currGen++;
            }
        }
    }

    firstRender = false;

    auto fpitch = calculateVoicePitch();

    auto [firstIndex, lastIndex] = sampleIndexRange();
    if (firstIndex >= 0)
    {
        for (auto i = firstIndex; i < lastIndex; ++i)
        {
            assert(i >= 0);
            assert(i < zone->samplePointers.size());
            if (zone->samplePointers[i])
            {
                calculateGeneratorRatio(fpitch, i, i - firstIndex);
            }
        }
    }

    if (useOversampling)
        for (auto i = 0; i < numGeneratorsActive; ++i)
            GD[i].ratio = GD[i].ratio >> 1;
    fpitch -= 69;

    memset(output, 0, sizeof(output));
    if (numGeneratorsActive > 0)
    {
        isAnyGeneratorRunning = false;
        float loutput alignas(16)[2][blockSize << 2];

        bool inloop = false;
        currentLoopPercentageF = 0.f;

        for (auto idx = firstIndex; idx < lastIndex; ++idx)
        {
            auto &variantData = zone->variantData.variants[idx];

            auto gidx = idx - firstIndex;
            if (isGeneratorRunning[gidx])
            {
                auto &s = zone->samplePointers[idx];
                assert(s);

                if (gidx == 0)
                {
                    GDIO[gidx].outputL = output[0];
                    GDIO[gidx].outputR = output[1];
                }
                else
                {
                    GDIO[gidx].outputL = loutput[0];
                    GDIO[gidx].outputR = loutput[1];
                }
                GD[gidx].sampleStart = 0;
                GD[gidx].sampleStop = s->sampleLengthPerChannel;

                /*
                 * We implement loop for count by gating on loop count
                 */
                if (variantData.loopMode == engine::Zone::LOOP_COUNT)
                {
                    // first loop is zero so don't skip -1
                    GD[gidx].gated = GD[gidx].loopCount < variantData.loopCountWhenCounted - 1;
                }
                else
                {
                    GD[gidx].gated = isGated;
                }
                GD[gidx].loopInvertedBounds =
                    1.f / std::max(1, GD[gidx].loopUpperBound - GD[gidx].loopLowerBound);
                GD[gidx].playbackInvertedBounds =
                    1.f / std::max(1, GD[gidx].playbackUpperBound - GD[gidx].playbackLowerBound);

                if (!GD[gidx].isFinished && Generator[gidx])
                {
                    Generator[gidx](&GD[gidx], &GDIO[gidx]);
                }

                inloop = inloop || GD[gidx].isInLoop;

                if (GD[gidx].isInLoop)
                {
                    currentLoopPercentageF = 1.0 * (GD[gidx].samplePos - GD[gidx].loopLowerBound) /
                                             (GD[gidx].loopUpperBound - GD[gidx].loopLowerBound);
                }
                currentSamplePercentageF =
                    1.0 * (GD[gidx].samplePos - GD[gidx].playbackLowerBound) /
                    (GD[gidx].playbackUpperBound - GD[gidx].playbackLowerBound);

                isGeneratorRunning[gidx] = !GD[gidx].isFinished;
                isAnyGeneratorRunning = isAnyGeneratorRunning || isGeneratorRunning[gidx];

                if (variantData.normalizationAmplitude != 1.0 || variantData.amplitude != 1.0)
                {
                    auto normBy = variantData.normalizationAmplitude * variantData.amplitude *
                                  variantData.amplitude * variantData.amplitude;
                    auto *sl = (gidx == 0 ? output[0] : loutput[0]);
                    auto *sr = (gidx == 0 ? output[1] : loutput[1]);
                    if (monoGenerator[gidx])
                    {
                        mech::scale_by<scxt::blockSize << (OS ? 1 : 0)>(normBy, sl);
                    }
                    else
                    {
                        mech::scale_by<scxt::blockSize << (OS ? 1 : 0)>(normBy, sl, sr);
                    }
                }

                bool panOverridesMono{false};
                if (variantData.pan > 0.001f || variantData.pan < -0.001f)
                {
                    namespace pl = sst::basic_blocks::dsp::pan_laws;

                    panOverridesMono = true;
                    auto pv = std::clamp(variantData.pan, -1.f, 1.f) * 0.5 + 0.5;
                    pl::panmatrix_t pmat{1, 1, 0, 0};

                    auto *sl = (gidx == 0 ? output[0] : loutput[0]);
                    auto *sr = (gidx == 0 ? output[1] : loutput[1]);
                    if (monoGenerator[gidx])
                    {
                        pl::monoEqualPowerUnityGainAtExtrema(pv, pmat);
                        for (int i = 0; i < blockSize << (OS ? 1 : 0); ++i)
                        {
                            sr[i] = sl[i] * pmat[3];
                            sl[i] = sl[i] * pmat[0];
                        }
                    }
                    else
                    {
                        pl::stereoEqualPower(pv, pmat);

                        for (int i = 0; i < blockSize << (forceOversample ? 1 : 0); ++i)
                        {
                            auto il = sl[i];
                            auto ir = sr[i];
                            sl[i] = pmat[0] * il + pmat[2] * ir;
                            sr[i] = pmat[1] * ir + pmat[3] * il;
                        }
                    }
                }

                if (gidx == 0)
                {
                    if (monoGenerator[gidx] && !allGeneratorsMono && !panOverridesMono)
                    {
                        mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(output[0], output[1]);
                    }
                }
                else
                {
                    mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(loutput[0],
                                                                              output[0]);
                    if (!monoGenerator[gidx] && !panOverridesMono)
                    {
                        mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(loutput[1],
                                                                                  output[1]);
                    }
                    else if (!allGeneratorsMono || panOverridesMono)
                    {
                        mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(loutput[0],
                                                                                  output[1]);
                    }
                }
            }
        }

        inLoopF = (inloop ? 1.f : 0.f);
        loopCountF =
            (inloop &&
                     zone->variantData.variants[sampleIndex].loopMode == engine::Zone::LOOP_COUNT &&
                     zone->variantData.variants[sampleIndex].loopCountWhenCounted > 0
                 ? ((float)GD[0].loopCount /
                    zone->variantData.variants[sampleIndex].loopCountWhenCounted)
                 : 0.f);

        if (useOversampling && !OS)
        {
            halfRate.process_block_D2(output[0], output[1], blockSize << 1);
        }
    }

    float tempbuf alignas(16)[2][BLOCK_SIZE << 1], postfader_buf alignas(16)[2][BLOCK_SIZE << 1];

    /*
     * Alright so time to document the logic. Remember all processors are required to do
     * stereo to stereo but may optionally do mono to mono or mono to stereo. Also this is
     * just for the linear routing case now. revisit this when we add the other routings.
     *
     * chainIsMono is the current state of mono down the chain.
     * If you get to processor[i] and chain is mono
     *    if the processor consumes mono and produces mono process mono and fade_1_block
     *    if the processor consumes mono and produces stereo process mono and fade_2 block and
     * toggle
     *    if the processor cannot consume mono, copy and proceed and toggle chainIsMon
     */
    bool chainIsMono{allGeneratorsMono};

    /*
     * Implement Sample Pan
     */
    auto pvz = *endpoints->mappingTarget.panP;
    if (pvz != 0.f)
    {
        samplePan.set_target(pvz);
        panOutputsBy(chainIsMono, samplePan);
        chainIsMono = false;
    }
    else if (chainIsMono)
    {
        // panning drops us by a root 2, so no panning needs to also
        constexpr float invsqrt2{1.f / 1.414213562373095};
        mech::mul_block<blockSize << (OS ? 1 : 0)>(output[0], invsqrt2);
    }

    auto pva = dsp::dbTable.dbToLinear(*endpoints->mappingTarget.ampP);

    if constexpr (OS)
    {
        sampleAmpOS.set_target(pva);
        if (chainIsMono)
        {
            sampleAmpOS.multiply_block(output[0]);
        }
        else
        {
            sampleAmpOS.multiply_2_blocks(output[0], output[1]);
        }
    }
    else
    {
        sampleAmp.set_target(pva);
        if (chainIsMono)
        {
            sampleAmp.multiply_block(output[0]);
        }
        else
        {
            sampleAmp.multiply_2_blocks(output[0], output[1]);
        }
    }

#define CALL_ROUTE(FNN)                                                                            \
    if (chainIsMono)                                                                               \
    {                                                                                              \
        if constexpr (OS)                                                                          \
        {                                                                                          \
            scxt::dsp::processor::FNN<OS, false>(fpitch, processors, processorConsumesMono,        \
                                                 processorMixOS, processorLevelOS,                 \
                                                 endpoints.get(), chainIsMono, output);            \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            scxt::dsp::processor::FNN<OS, false>(fpitch, processors, processorConsumesMono,        \
                                                 processorMix, processorLevel, endpoints.get(),    \
                                                 chainIsMono, output);                             \
        }                                                                                          \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        if constexpr (OS)                                                                          \
        {                                                                                          \
            scxt::dsp::processor::FNN<OS, true>(fpitch, processors, processorConsumesMono,         \
                                                processorMixOS, processorLevelOS, endpoints.get(), \
                                                chainIsMono, output);                              \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            scxt::dsp::processor::FNN<OS, true>(fpitch, processors, processorConsumesMono,         \
                                                processorMix, processorLevel, endpoints.get(),     \
                                                chainIsMono, output);                              \
        }                                                                                          \
    }

    /*
     * This is all voice time type reset logic basically.
     */
    bool hasProcs{false};
    for (int i = 0; i < processorsPerZoneAndGroup; ++i)
    {
        auto proct = processors[i] ? processors[i]->getType() : dsp::processor::proct_none;
        if (zone->processorStorage[i].type != proct)
        {
            if (processors[i])
                dsp::processor::unspawnProcessor(processors[i]);
            processors[i] = nullptr;
            proct = zone->processorStorage[i].type;
        }

        if (!processors[i] && zone->processorStorage[i].isActive &&
            zone->processorStorage[i].type != dsp::processor::proct_none)
        {
            processorType[i] = proct;
            // this is copied below in the init.
            processors[i] = dsp::processor::spawnProcessorInPlace(
                processorType[i], zone->getEngine(), zone->getEngine()->getMemoryPool().get(),
                processorPlacementStorage[i], dsp::processor::processorMemoryBufferSize,
                zone->processorStorage[i], endpoints->processorTarget[i].fp, processorIntParams[i],
                forceOversample, false);

            processors[i]->setSampleRate(sampleRate * (forceOversample ? 2 : 1));
            processors[i]->setTempoPointer(&(zone->getEngine()->transport.tempo));

            processors[i]->init();
            processors[i]->setKeytrack(zone->processorStorage[i].isKeytracked);

            processorConsumesMono[i] = allGeneratorsMono && processors[i]->canProcessMono();
        }
        if (processors[i])
        {
            memcpy(&processorIntParams[i][0], zone->processorStorage[i].intParams.data(),
                   sizeof(processorIntParams[i]));
            processors[i]->bypassAnyway = !zone->processorStorage[i].isActive;
        }

        hasProcs = hasProcs || processors[i];
    }

    if (hasProcs)
    {
        switch (zone->outputInfo.procRouting)
        {
        case engine::HasGroupZoneProcessors<engine::Zone>::procRoute_linear:
        {
            CALL_ROUTE(processSequential);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Zone>::procRoute_ser2:
        {
            CALL_ROUTE(processSer2Pattern);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Zone>::procRoute_ser3:
        {
            CALL_ROUTE(processSer3Pattern);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Zone>::procRoute_par1:
        {
            CALL_ROUTE(processPar1Pattern);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Zone>::procRoute_par2:
        {
            CALL_ROUTE(processPar2Pattern);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Zone>::procRoute_par3:
        {
            CALL_ROUTE(processPar3Pattern);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Zone>::procRoute_bypass:
            break;
        }
    }
    /*
     * Implement output pan
     */
    auto pvo = std::clamp(*endpoints->outputTarget.panP +
                              2.0 * (noteExpressions[(int)ExpressionIDs::PAN] - 0.5),
                          -1., 1.);
    if (pvo != 0.f)
    {
        outputPan.set_target(pvo);
        panOutputsBy(chainIsMono, outputPan);
        chainIsMono = false;
    }

    auto pao = *endpoints->outputTarget.ampP;

    auto velFac = zone->parentGroup->outputInfo.velocitySensitivity;
    auto velMul = std ::clamp((1 - velFac) + velFac * velocity, 0.f, 1.f);

    pao *= velMul;

    pao *= velKeyFade;

    if (terminationSequence > 0)
    {
        terminationSequence--;
        pao *= terminationSequence / blocksToTerminate;
    }
    pao = std::max(pao, 0.f);

    if (zone->parentGroup->outputInfo.muted)
    {
        pao = 0.f;
    }

    if constexpr (OS)
    {
        outputAmpOS.set_target(pao * pao * pao * noteExpressions[(int)ExpressionIDs::VOLUME]);
        if (chainIsMono)
        {
            outputAmpOS.multiply_block(output[0]);
        }
        else
        {
            outputAmpOS.multiply_2_blocks(output[0], output[1]);
        }
    }
    else
    {
        outputAmp.set_target(pao * pao * pao * noteExpressions[(int)ExpressionIDs::VOLUME]);
        if (chainIsMono)
        {
            outputAmp.multiply_block(output[0]);
        }
        else
        {
            outputAmp.multiply_2_blocks(output[0], output[1]);
        }
    }

    if constexpr (OS)
    {
        // At the end of the voice we have to produce stereo
        if (chainIsMono)
        {
            mech::scale_by<blockSize << 1>(aegOS.outputCache, output[0]);
            mech::copy_from_to<blockSize << 1>(output[0], output[1]);
        }
        else
        {
            mech::scale_by<blockSize << 1>(aegOS.outputCache, output[0], output[1]);
        }
    }
    else
    {
        // At the end of the voice we have to produce stereo
        if (chainIsMono)
        {
            mech::scale_by<blockSize>(aeg.outputCache, output[0]);
            mech::copy_from_to<blockSize>(output[0], output[1]);
        }
        else
        {
            mech::scale_by<blockSize>(aeg.outputCache, output[0], output[1]);
        }
    }

    /*
     * Finally do voice state update
     */
    if (isAEGRunning && (hasProcs || isAnyGeneratorRunning))
    {
        isVoicePlaying = true;
    }
    else
    {
        SCLOG_IF(voiceLifecycle, "Voice terminating due to AEG at " << SCD(key));

        isVoicePlaying = false;
    }

    if (terminationSequence == 0)
    {
        SCLOG_IF(voiceLifecycle, "Voice terminating due to termiantion sequence " << SCD(key));

        isVoicePlaying = false;
    }
    return true;
}

void Voice::panOutputsBy(bool chainIsMono, const lipol &plip)
{
    namespace pl = sst::basic_blocks::dsp::pan_laws;
    // For now we don't interpolate over the block for pan
    auto pv = (std::clamp(plip.target, -1.f, 1.f) + 1) * 0.5;
    pl::panmatrix_t pmat{1, 1, 0, 0};
    if (chainIsMono)
    {
        chainIsMono = false;
        pl::monoEqualPowerUnityGainAtExtrema(pv, pmat);
        for (int i = 0; i < blockSize << (forceOversample ? 1 : 0); ++i)
        {
            output[1][i] = output[0][i] * pmat[3];
            output[0][i] = output[0][i] * pmat[0];
        }
    }
    else
    {
        pl::stereoEqualPower(pv, pmat);

        for (int i = 0; i < blockSize << (forceOversample ? 1 : 0); ++i)
        {
            auto il = output[0][i];
            auto ir = output[1][i];
            output[0][i] = pmat[0] * il + pmat[2] * ir;
            output[1][i] = pmat[1] * ir + pmat[3] * il;
        }
    }
}

void Voice::initializeGenerator()
{
    numGeneratorsActive = 0;
    allGeneratorsMono = true;
    isAnyGeneratorRunning = false;

    if (sampleIndex < 0)
    {
        return;
    }

    // but the default of course is to use the sample index
    auto [firstIndex, lastIndex] = sampleIndexRange();

    for (auto i = firstIndex; i < lastIndex; ++i)
    {
        if (zone->samplePointers[i]->isMissingPlaceholder)
        {
            return;
        }
    }
    numGeneratorsActive = lastIndex - firstIndex;

    int currGen{0};
    allGeneratorsMono = true;
    for (auto currIndex = firstIndex; currIndex < lastIndex; currIndex++)
    {
        auto &s = zone->samplePointers[currIndex];

        auto &variantData = zone->variantData.variants[currIndex];

        GDIO[currGen].outputL = output[0];
        GDIO[currGen].outputR = output[1];
        if (s->bitDepth == sample::Sample::BD_I16)
        {
            GDIO[currGen].sampleDataL = s->GetSamplePtrI16(0);
            GDIO[currGen].sampleDataR = s->GetSamplePtrI16(1);
        }
        else if (s->bitDepth == sample::Sample::BD_F32)
        {
            GDIO[currGen].sampleDataL = s->GetSamplePtrF32(0);
            GDIO[currGen].sampleDataR = s->GetSamplePtrF32(1);
        }
        else
        {
            assert(false);
        }
        GDIO[currGen].waveSize = s->sampleLengthPerChannel;

        GD[currGen].samplePos = variantData.startSample;
        GD[currGen].sampleSubPos = 0;
        GD[currGen].loopLowerBound = variantData.startSample;
        GD[currGen].loopUpperBound = variantData.endSample;
        GD[currGen].loopFade = variantData.loopFade;
        GD[currGen].playbackLowerBound = variantData.startSample;
        GD[currGen].playbackUpperBound = variantData.endSample;
        GD[currGen].direction = 1;
        GD[currGen].isFinished = false;

        if (variantData.loopActive)
        {
            GD[currGen].loopLowerBound = variantData.startLoop;
            GD[currGen].loopUpperBound = variantData.endLoop;
        }

        if (variantData.playReverse)
        {
            GD[currGen].samplePos = GD[currGen].playbackUpperBound;
            GD[currGen].direction = -1;
        }
        GD[currGen].directionAtOutset = GD[currGen].direction;

        calculateGeneratorRatio(calculateVoicePitch(), currIndex, currGen);

        // TODO: This constant came from SC. Wonder why it is this value. There was a comment
        // comparing with 167777216 so any speedup at all.
        useOversampling = std::abs(GD[currGen].ratio) > 18000000 || forceOversample;
        GD[currGen].blockSize = blockSize * (useOversampling ? 2 : 1);

        if (!forceOversample && (variantData.interpolationType == dsp::ZeroOrderHold ||
                                 variantData.interpolationType == dsp::ZOHAA))
            useOversampling = false;

        Generator[currGen] = nullptr;

        monoGenerator[currGen] = s->channels == 1;
        allGeneratorsMono = allGeneratorsMono && monoGenerator[currGen] &&
                            (variantData.pan < 0.01f && variantData.pan > -0.01f);
        Generator[currGen] = dsp::GetFPtrGeneratorSample(
            !monoGenerator[currGen], s->bitDepth == sample::Sample::BD_F32, variantData.loopActive,
            variantData.loopDirection == engine::Zone::FORWARD_ONLY,

            // We doo loop count by gating on loopCount < maxLoopCount
            variantData.loopMode == engine::Zone::LOOP_WHILE_GATED ||
                variantData.loopMode == engine::Zone::LOOP_COUNT);
        SCLOG_IF(generatorInitialization,
                 "Generator : " << SCD(currGen) << SCD((size_t)Generator[currGen]));
        SCLOG_IF(generatorInitialization, "     SMP  : " << SCD(GDIO[currGen].sampleDataL)
                                                         << SCD(GDIO[currGen].sampleDataR));
        SCLOG_IF(generatorInitialization, "     SLE  : " << SCD(GDIO[currGen].waveSize));

        GD[currGen].interpolationType = variantData.interpolationType;
        isGeneratorRunning[currGen] = true;
        isAnyGeneratorRunning = true;

        currGen++;
    }
}

float Voice::calculateVoicePitch()
{
    auto kd = (key - zone->mapping.rootKey) * zone->mapping.tracking + zone->mapping.rootKey;

    auto fpitch = kd + *endpoints->mappingTarget.pitchOffsetP;
    auto pitchWheel = zone->parentGroup->parentPart->pitchBendValue;
    auto pu = zone->mapping.pbUp;
    auto pd = zone->mapping.pbDown;
    if (pu < 0)
        pu = zone->parentGroup->outputInfo.pbUp;
    if (pd < 0)
        pd = zone->parentGroup->outputInfo.pbDown;

    auto pitchMv = pitchWheel > 0 ? pu : pd;
    fpitch += pitchWheel * pitchMv;

    fpitch += zone->parentGroup->outputInfo.tuning;

    float retuner{retunedKeyAtAttack};
    if (retuneContinuous)
        retuner =
            zone->getEngine()->midikeyRetuner.retuneRemappedKey(channel, key, originalMidiKey);

    fpitch += retuner;
    fpitch += noteExpressions[(int)ExpressionIDs::TUNING];
    fpitch += mpePitchBend;

    keytrackPerOct = (key + retuner - zone->mapping.rootKey) / 12.0;
    return fpitch;
}

void Voice::calculateGeneratorRatio(float pitch, int cSampleIndex, int generatorIndex)
{
#if 0
    keytrack = (fkey + zone->pitch_bend_depth * ctrl[c_pitch_bend] - 60.0f) *
               (1 / 12.0f); // keytrack as modulation source
    float kt = (playmode == pm_forward_hitpoints) ? 0 : ((fkey - zone->key_root) * zone->keytrack);

    fpitch = part->transpose + zone->transpose + zone->finetune +
             zone->pitch_bend_depth * ctrl[c_pitch_bend] + mm.get_destination_value(md_pitch);

    // not important enough to sacrifice perf for.. better to use on demand in case
    // loop_pos = limit_range((sample_pos -
    // mm.get_destination_value_int(md_loop_start))/(float)mm.get_destination_value_int(md_loop_length),0,1);

    GD.ratio = Float2Int((float)((wave->sample_rate * samplerate_inv) * 16777216.f *
                                 note_to_pitch(fpitch + kt - zone->pitchcorrection) *
                                 mm.get_destination_value(md_rate)));
    fpitch += fkey - 69.f; // relative to A3 (440hz)
    // TODO gross for now - correct
    float ndiff = pitch - zone->mapping.rootKey;
    auto fac = tuning::equalTuning.note_to_pitch(ndiff);

    // TODO round robin
    GD.ratio = (int32_t)((1 << 24) * fac * zone->samplePointers[sampleIndex]->sample_rate * sampleRateInv *
                         (1.0 + modMatrix.getValue(modulation::vmd_Sample_Playback_Ratio, 0)));
#endif
    auto &var = zone->variantData.variants[sampleIndex];
    // pitch already has keytrack in
    auto kd = (pitch - zone->mapping.rootKey);

    float ndiff = kd + zone->parentGroup->parentPart->configuration.tuning + var.pitchOffset;

    auto fac = tuning::equalTuning.note_to_pitch(ndiff);

    GD[generatorIndex].ratio =
        (int32_t)((1 << 24) * fac * zone->samplePointers[cSampleIndex]->sample_rate *
                  sampleRateInv * (1.0 + *endpoints->mappingTarget.playbackRatioP));
}

void Voice::initializeProcessors()
{
    assert(zone);
    assert(zone->getEngine());
    samplePan.set_target_instant(*endpoints->mappingTarget.panP);
    sampleAmp.set_target_instant(*endpoints->mappingTarget.ampP);

    outputPan.set_target_instant(*endpoints->outputTarget.panP);
    outputAmp.set_target_instant(*endpoints->outputTarget.ampP);

    auto fpitch = calculateVoicePitch();
    for (auto i = 0; i < engine::processorCount; ++i)
    {
        processorIsActive[i] = zone->processorStorage[i].isActive;
        processorMix[i].set_target_instant(*endpoints->processorTarget[i].mixP);
        processorMixOS[i].set_target_instant(*endpoints->processorTarget[i].mixP);

        auto ol = *endpoints->processorTarget[i].outputLevelDbP;
        ol = ol * ol * ol * dsp::processor::ProcessorStorage::maxOutputAmp;

        processorLevel[i].set_target_instant(ol);
        processorLevelOS[i].set_target_instant(ol);

        processorType[i] = zone->processorStorage[i].type;

        memcpy(&processorIntParams[i][0], zone->processorStorage[i].intParams.data(),
               sizeof(processorIntParams[i]));

        if ((processorIsActive[i] && processorType[i] != dsp::processor::proct_none) ||
            (processorType[i] == dsp::processor::proct_none && !processorIsActive[i]))
        {
            // this init code is partly copied above in the voice state toggle
            processors[i] = dsp::processor::spawnProcessorInPlace(
                processorType[i], zone->getEngine(), zone->getEngine()->getMemoryPool().get(),
                processorPlacementStorage[i], dsp::processor::processorMemoryBufferSize,
                zone->processorStorage[i], endpoints->processorTarget[i].fp, processorIntParams[i],
                forceOversample, false);
            // the processor may still be NULL here!
            // Don't touch it until the safety-checked block below
        }
        else
        {
            processors[i] = nullptr;
        }

        if (processors[i])
        {
            processors[i]->init_pitch(fpitch - 69);

            processors[i]->setSampleRate(sampleRate * (forceOversample ? 2 : 1));
            processors[i]->setTempoPointer(&(zone->getEngine()->transport.tempo));

            processors[i]->init();
            processors[i]->setKeytrack(zone->processorStorage[i].isKeytracked);

            processorConsumesMono[i] = allGeneratorsMono && processors[i]->canProcessMono();
        }
    }
}

void Voice::updateTransportPhasors()
{
    auto bt = engine->transport.timeInBeats - startBeat;
#if 0
    float mul = 1 << ((numTransportPhasors - 1) / 2);
    for (int i = 0; i < numTransportPhasors; ++i)
    {
        float rawBeat;
        transportPhasors[i] = std::modf((float)(bt)*mul, &rawBeat);
        mul = mul / 2;
    }
#endif
}

void Voice::onSampleRateChanged()
{
    setHasModulatorsSampleRate(samplerate, samplerate_inv);
    noteExpressionLags.setRateInMilliseconds(1000 * 64 / 48000, samplerate, blockSizeInv);
    noteExpressionLags.snapAllActiveToTarget();
}

std::pair<int16_t, int16_t> Voice::sampleIndexRange() const
{
    int16_t firstIndex{-1}, lastIndex{-1};

    if (zone->variantData.variantPlaybackMode == engine::Zone::UNISON)
    {
        lastIndex = 0;

        for (int i = 0; i < maxVariantsPerZone; ++i)
        {
            if (zone->variantData.variants[i].active)
            {
                firstIndex = 0;
                lastIndex++;
            }
        }
    }
    else
    {
        firstIndex = sampleIndex;
        lastIndex = sampleIndex + 1;
    }

    return {firstIndex, lastIndex};
}
} // namespace scxt::voice
