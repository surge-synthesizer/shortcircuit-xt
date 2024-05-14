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

#include "voice.h"
#include "tuning/equal.h"
#include <cassert>
#include <cmath>

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/PanLaws.h"
#include "engine/engine.h"

namespace scxt::voice
{

Voice::Voice(engine::Engine *e, engine::Zone *z)
    : scxt::modulation::shared::HasModulators<Voice>(this), engine(e), zone(z),
      sampleIndex(zone->sampleIndex), halfRate(6, true), endpoints(nullptr) // see comment
{
    assert(zone);
    memset(output, 0, 2 * blockSize * sizeof(float));
    memset(processorIntParams, 0, sizeof(processorIntParams));
}

Voice::~Voice()
{
#if BUILD_IS_DEBUG
    if (isVoiceAssigned)
    {
        SCLOG("WARNING: Destroying assigned voice. (OK in shutdown)");
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
    forceOversample = zone->parentGroup->outputInfo.oversample;

    for (auto i = 0U; i < engine::lfosPerZone; ++i)
    {
        const auto &ms = zone->modulatorStorage[i];
        lfoEvaluator[i] = ms.isStep() ? STEP : (ms.isEnv() ? ENV : (ms.isMSEG() ? MSEG : CURVE));
    }

    startBeat = engine->transport.timeInBeats;
    updateTransportPhasors();

    // This order matters
    endpoints->sources.bind(modMatrix, *zone, *this);
    modMatrix.prepare(zone->routingTable);
    endpoints->bindTargetBaseValues(modMatrix, *zone);
    modMatrix.process();

    // These probably need to happen after the modulator is set up
    initializeGenerator();
    initializeProcessors();

    for (auto i = 0U; i < engine::lfosPerZone; ++i)
    {
        const auto &ms = zone->modulatorStorage[i];
        if (lfoEvaluator[i] == STEP)
        {
            stepLfos[i].setSampleRate(sampleRate, sampleRateInv);

            stepLfos[i].assign(&zone->modulatorStorage[i], endpoints->lfo[i].rateP, nullptr,
                               engine->rngGen);
        }
        else if (lfoEvaluator[i] == CURVE)
        {
            curveLfos[i].setSampleRate(sampleRate, sampleRateInv);
            curveLfos[i].attack(ms.start_phase, ms.modulatorShape);
        }
        else if (lfoEvaluator[i] == ENV)
        {
            envLfos[i].setSampleRate(sampleRate, sampleRateInv);
            envLfos[i].attack(*endpoints->lfo[i].env.delayP);
        }
        else
        {
            SCLOG("Unimplemented modulator shape " << ms.modulatorShape);
        }
    }

    aeg.attackFrom(0.0); // TODO Envelope Legato Mode
    eg2.attackFrom(0.0);
    if (forceOversample)
    {
        aegOS.attackFrom(0.0);
    }

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
    // TODO round robin state
    auto &s = zone->samplePointers[sampleIndex];
    auto &sdata = zone->sampleData.samples[sampleIndex];
    assert(s);

    // Run Modulators - these run at base rate never oversampled
    for (auto i = 0; i < engine::lfosPerZone; ++i)
    {
        if (lfoEvaluator[i] == STEP)
        {
            stepLfos[i].process(blockSize);
        }
        else if (lfoEvaluator[i] == CURVE)
        {
            auto &lp = endpoints->lfo[i];

            // SCLOG(zone->modulatorStorage[0].curveLfoStorage.delay << " " << *lp.curveDelayP);
            curveLfos[i].process(*lp.rateP, *lp.curve.deformP, *lp.curve.delayP, *lp.curve.attackP,
                                 *lp.curve.releaseP,
                                 zone->modulatorStorage[i].curveLfoStorage.useenv,
                                 zone->modulatorStorage[i].curveLfoStorage.unipolar, isGated);
        }
        else if (lfoEvaluator[i] == ENV)
        {
            auto &lp = endpoints->lfo[i].env;

            envLfos[i].process(*lp.delayP, *lp.attackP, *lp.holdP, *lp.decayP, *lp.sustainP,
                               *lp.releaseP, isGated);
        }
        else
        {
        }
    }

    // TODO probably want to process LFO modulations at this point so the AEG and EG2
    // are modulatable

    // TODO SHape Fixes
    bool envGate = (sdata.playMode == engine::Zone::NORMAL && isGated) ||
                   (sdata.playMode == engine::Zone::ONE_SHOT && isGeneratorRunning) ||
                   (sdata.playMode == engine::Zone::ON_RELEASE && isGeneratorRunning);

    auto &aegp = endpoints->aeg;
    if constexpr (OS)
    {
        // we need the aegOS for the curve in oversample space
        aegOS.processBlock(*aegp.aP, *aegp.hP, *aegp.dP, *aegp.sP, *aegp.rP, *aegp.asP, *aegp.dsP,
                           *aegp.rsP, envGate);
    }

    // But We need to run the undersample AEG no matter what since it is a modulatino source
    aeg.processBlock(*aegp.aP, *aegp.hP, *aegp.dP, *aegp.sP, *aegp.rP, *aegp.asP, *aegp.dsP,
                     *aegp.rsP, envGate);
    // TODO: And output is non zero once we are past attack
    isAEGRunning = (aeg.stage != ahdsrenv_t ::s_complete);

    auto &eg2p = endpoints->eg2;
    eg2.processBlock(*eg2p.aP, *eg2p.hP, *eg2p.dP, *eg2p.sP, *eg2p.rP, *eg2p.asP, *eg2p.dsP,
                     *eg2p.rsP, envGate);

    updateTransportPhasors();

    // TODO and probably just want to process the envelopes here
    modMatrix.process();

    auto fpitch = calculateVoicePitch();
    calculateGeneratorRatio(fpitch);
    if (useOversampling)
        GD.ratio = GD.ratio >> 1;
    fpitch -= 69;

    // TODO : Start and End Points
    GD.sampleStart = 0;
    GD.sampleStop = s->sample_length;

    GD.gated = isGated;
    GD.loopInvertedBounds = 1.f / std::max(1, GD.loopUpperBound - GD.loopLowerBound);
    GD.playbackInvertedBounds = 1.f / std::max(1, GD.playbackUpperBound - GD.playbackLowerBound);
    if (!GD.isFinished)
    {
        Generator(&GD, &GDIO);

        if (useOversampling && !OS)
        {
            halfRate.process_block_D2(output[0], output[1], blockSize << 1);
        }
    }
    else
        memset(output, 0, sizeof(output));

    // TODO Ringout - this is obvioulsy wrong
    if (GD.isFinished)
    {
        isGeneratorRunning = false;
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
    bool chainIsMono{monoGenerator};

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

    auto pva = *endpoints->mappingTarget.ampP;

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

    for (auto i = 0; i < engine::processorCount; ++i)
    {
        // TODO: FixMe for oversample
        if (processors[i])
        {
            auto mix = [this, i]() {
                if constexpr (OS)
                {
                    return &processorMixOS[i];
                }
                else
                {
                    return &processorMix[i];
                }
            };
            endpoints->processorTarget[i].snapValues();
            // TODO: Snap int values here (and same in group)

            mix()->set_target(*endpoints->processorTarget[i].mixP);

            if (chainIsMono && processorConsumesMono[i] &&
                !processors[i]->monoInputCreatesStereoOutput())
            {
                // mono to mono
                processors[i]->process_monoToMono(output[0], tempbuf[0], fpitch);
                mix()->fade_blocks(output[0], tempbuf[0], output[0]);
            }
            else if (chainIsMono && processorConsumesMono[i])
            {
                assert(processors[i]->monoInputCreatesStereoOutput());
                // mono to stereo. process then toggle
                processors[i]->process_monoToStereo(output[0], tempbuf[0], tempbuf[1], fpitch);
                mix()->fade_blocks(output[0], tempbuf[0], output[0]);
                mix()->fade_blocks(output[0], tempbuf[1], output[1]);
                // this out[0] is NOT a typo. Input is mono

                chainIsMono = false;
            }
            else if (chainIsMono)
            {
                assert(!processorConsumesMono[i]);
                // stereo to stereo. copy L to R then process
                mech::copy_from_to<blockSize << (OS ? 1 : 0)>(output[0], output[1]);
                chainIsMono = false;
                processors[i]->process_stereo(output[0], output[1], tempbuf[0], tempbuf[1], fpitch);
                mix()->fade_blocks(output[0], tempbuf[0], output[0]);
                mix()->fade_blocks(output[1], tempbuf[1], output[1]);
            }
            else
            {
                // stereo to stereo. process
                processors[i]->process_stereo(output[0], output[1], tempbuf[0], tempbuf[1], fpitch);
                mix()->fade_blocks(output[0], tempbuf[0], output[0]);
                mix()->fade_blocks(output[1], tempbuf[1], output[1]);
            }
            // TODO: What was the filter_modout? Seems SC2 never finished it
            /*
            filter_modout[0] = voice_filter[0]->modulation_output;
                                   */
        }
    }

    /*
     * Implement output pan
     */
    auto pvo = *endpoints->outputTarget.panP;
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

    if constexpr (OS)
    {
        outputAmpOS.set_target(pao * pao * pao);
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
        outputAmp.set_target(pao * pao * pao);
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
    if (isAEGRunning)
        isVoicePlaying = true;
    else
        isVoicePlaying = false;

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
        for (int i = 0; i < blockSize << forceOversample; ++i)
        {
            output[1][i] = output[0][i] * pmat[3];
            output[0][i] = output[0][i] * pmat[0];
        }
    }
    else
    {
        pl::stereoEqualPower(pv, pmat);

        for (int i = 0; i < blockSize << forceOversample; ++i)
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
    // TODO round robin
    auto &s = zone->samplePointers[sampleIndex];
    auto &sampleData = zone->sampleData.samples[sampleIndex];
    assert(s);

    GDIO.outputL = output[0];
    GDIO.outputR = output[1];
    GDIO.sampleDataL = s->sampleData[0];
    GDIO.sampleDataR = s->sampleData[1];
    GDIO.waveSize = s->sample_length;

    GD.samplePos = sampleData.startSample;
    GD.sampleSubPos = 0;
    GD.loopLowerBound = sampleData.startSample;
    GD.loopUpperBound = sampleData.endSample;
    GD.loopFade = sampleData.loopFade;
    GD.playbackLowerBound = sampleData.startSample;
    GD.playbackUpperBound = sampleData.endSample;
    GD.direction = 1;
    GD.isFinished = false;

    if (sampleData.loopActive)
    {
        GD.loopLowerBound = sampleData.startLoop;
        GD.loopUpperBound = sampleData.endLoop;
    }

    if (sampleData.playReverse)
    {
        GD.samplePos = GD.playbackUpperBound;
        GD.direction = -1;
    }
    GD.directionAtOutset = GD.direction;

    calculateGeneratorRatio(calculateVoicePitch());

    // TODO: This constant came from SC. Wonder why it is this value. There was a comment comparing
    // with 167777216 so any speedup at all.
    useOversampling = std::abs(GD.ratio) > 18000000 || forceOversample;
    GD.blockSize = blockSize * (useOversampling ? 2 : 1);

    Generator = nullptr;

    monoGenerator = s->channels == 1;
    Generator = dsp::GetFPtrGeneratorSample(!monoGenerator, s->bitDepth == sample::Sample::BD_F32,
                                            sampleData.loopActive,
                                            sampleData.loopDirection == engine::Zone::FORWARD_ONLY,
                                            sampleData.loopMode == engine::Zone::LOOP_WHILE_GATED);
}

float Voice::calculateVoicePitch()
{
    auto fpitch = key + *endpoints->mappingTarget.pitchOffsetP;
    auto pitchWheel = zone->parentGroup->parentPart->pitchBendSmoother.output;
    auto pitchMv = pitchWheel > 0 ? zone->mapping.pbUp : zone->mapping.pbDown;
    fpitch += pitchWheel * pitchMv;

    fpitch += zone->getEngine()->midikeyRetuner.retuneRemappedKey(channel, key, originalMidiKey);

    return fpitch;
}

void Voice::calculateGeneratorRatio(float pitch)
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

    // TODO gross for now - correct
    float ndiff = pitch - zone->mapping.rootKey;
    auto fac = tuning::equalTuning.note_to_pitch(ndiff);
    // TODO round robin
    GD.ratio = (int32_t)((1 << 24) * fac * zone->samplePointers[sampleIndex]->sample_rate *
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

    for (auto i = 0; i < engine::processorCount; ++i)
    {
        processorIsActive[i] = zone->processorStorage[i].isActive;
        processorMix[i].set_target_instant(*endpoints->processorTarget[i].mixP);
        processorMixOS[i].set_target_instant(*endpoints->processorTarget[i].mixP);

        processorType[i] = zone->processorStorage[i].type;

        memcpy(&processorIntParams[i][0], zone->processorStorage[i].intParams.data(),
               sizeof(processorIntParams[i]));

        if (processorIsActive[i])
        {
            processors[i] = dsp::processor::spawnProcessorInPlace(
                processorType[i], zone->getEngine()->getMemoryPool().get(),
                processorPlacementStorage[i], dsp::processor::processorMemoryBufferSize,
                zone->processorStorage[i], endpoints->processorTarget[i].fp, processorIntParams[i],
                forceOversample, false);
        }
        else
        {
            processors[i] = nullptr;
        }
        if (processors[i])
        {
            processors[i]->setSampleRate(sampleRate * (forceOversample ? 2 : 1));
            processors[i]->init();
            processors[i]->setKeytrack(zone->processorStorage[i].isKeytracked);

            processorConsumesMono[i] = monoGenerator && processors[i]->canProcessMono();
        }
    }
}

void Voice::updateTransportPhasors()
{
    auto bt = engine->transport.timeInBeats - startBeat;
    float mul = 1 << ((numTransportPhasors - 1) / 2);
    for (int i = 0; i < numTransportPhasors; ++i)
    {
        float rawBeat;
        transportPhasors[i] = std::modf((float)(bt)*mul, &rawBeat);
        mul = mul / 2;
    }
}
} // namespace scxt::voice
