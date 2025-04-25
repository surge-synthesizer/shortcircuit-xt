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

#include "group.h"
#include "bus.h"
#include "part.h"

#include "sst/basic-blocks/simd/setup.h"

#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/PanLaws.h"

#include <cassert>

#include "messaging/messaging.h"
#include "patch.h"
#include "engine.h"
#include "group_and_zone_impl.h"
#include "dsp/processor/routing.h"

namespace scxt::engine
{

Group::Group()
    : id(GroupID::next()), name(id.to_string()), endpoints{nullptr},
      modulation::shared::HasModulators<Group, egsPerGroup>(this), osDownFilter(6, true)
{
}

void Group::rePrepareAndBindGroupMatrix()
{
    for (int i = 0; i < lfosPerGroup; ++i)
    {
        const auto &ms = modulatorStorage[i];
        lfoEvaluator[i] = ms.isStep() ? STEP : (ms.isEnv() ? ENV : (ms.isMSEG() ? MSEG : CURVE));
    }

    endpoints.sources.bind(modMatrix, *this);
    modMatrix.prepare(routingTable, getSampleRate(), blockSize);
    endpoints.bindTargetBaseValues(modMatrix, *this);
    modMatrix.process();

    std::fill(lfosActive.begin(), lfosActive.end(), false);
    egsActive[0] = false; // no AEG here
    egsActive[1] = false;

    auto updateInternalState = [this](const auto &source) {
        if (source.gid == 'greg')
        {
            if (source.tid == 'eg1 ')
            {
                egsActive[0] = true;
            }
            else
            {
                egsActive[1] = true;
            }
        }
        if (source.gid == 'grlf')
        {
            lfosActive[source.index] = true;
        }
    };
    for (auto route : routingTable.routes)
    {
        // Inactive or target free routes can be ignored
        if (!route.active || !route.target.has_value())
            continue;
        if (route.source.has_value())
            updateInternalState(*route.source);
        if (route.sourceVia.has_value())
            updateInternalState(*route.sourceVia);
    }
    for (auto &z : zones)
    {
        for (int i = 0; i < lfosPerGroup; ++i)
        {
            lfosActive[i] = lfosActive[i] || z->glfosActive[i];
        }
    }
}

void Group::process(Engine &e)
{
    if (outputInfo.oversample)
        processWithOS<true>(e);
    else
        processWithOS<false>(e);
}

template <bool OS> void Group::processWithOS(scxt::engine::Engine &e)
{
    assertSampleRateSet();

    /*
     * Groups have long lived runs so the processors need to reset etc...
     * when oversample toggles
     */
    if (lastOversample != outputInfo.oversample)
    {
        lastOversample = outputInfo.oversample;
        attack();
        for (int i = 0; i < engine::processorCount; ++i)
        {
            onProcessorTypeChanged(i, processorStorage[i].type);
        }
    }

    namespace blk = sst::basic_blocks::mechanics;

    mUILag.process();
    updateRetriggers(&endpoints);

    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));

    // When we have alternate group routing we change these pointers
    float *lOut = output[0];
    float *rOut = output[1];

    bool gated{attackInThisBlock};
    attackInThisBlock = false;
    for (const auto &z : zones)
    {
        gated = gated || (z->gatedVoiceCount > 0);
    }

    for (auto i = 0; i < engine::lfosPerGroup; ++i)
    {
        if (!lfosActive[i])
            continue;

        bool rt = doLFORetrigger[i];
        doLFORetrigger[i] = false;

        if (lfoEvaluator[i] == STEP)
        {
            if (rt)
            {
                stepLfos[i].retrigger();
            }
            stepLfos[i].process(blockSize);
        }
        else if (lfoEvaluator[i] == CURVE)
        {
            auto &lp = endpoints.lfo[i];

            if (rt)
            {
                curveLfos[i].attack(0, modulatorStorage[i].modulatorShape);
            }

            curveLfos[i].process(*lp.rateP, *lp.curve.deformP, *lp.curve.angleP, *lp.curve.delayP,
                                 *lp.curve.attackP, *lp.curve.releaseP,
                                 modulatorStorage[i].curveLfoStorage.useenv,
                                 modulatorStorage[i].curveLfoStorage.unipolar, gated);
        }
        else if (lfoEvaluator[i] == ENV)
        {
            auto &lp = endpoints.lfo[i].env;

            auto eloop = modulatorStorage[i].envLfoStorage.loop;
            auto useGate = gated;
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
        }
        else
        {
        }
    }
    phasorEvaluator.step(e.transport, miscSourceStorage);

    bool envGate = gated;
    for (int i = 0; i < egsPerGroup; ++i)
    {
        if (egsActive[i])
        {
            auto &aegp = endpoints.egTarget[i];
            if (doEGRetrigger[i])
            {
                doEGRetrigger[i] = false;
                eg[i].attackFromWithDelay(eg[i].outBlock0, *aegp.dlyP, *aegp.aP);
            }
            eg[i].processBlockWithDelay(*aegp.dlyP, *aegp.aP, *aegp.hP, *aegp.dP, *aegp.sP,
                                        *aegp.rP, *aegp.asP, *aegp.dsP, *aegp.rsP, envGate, false);
        }
    }

    modMatrix.process();

    auto oAZ = activeZones;
    rescanWeakRefs = 0;
    for (int i = 0; i < activeZones; ++i)
    {
        auto z = activeZoneWeakRefs[i];
        assert(z->isActive());
        z->process(e);
        assert(z->isActive() || rescanWeakRefs > 0);
        assert(oAZ == activeZones);

        /*
         * This is just an optimization to not accumulate. The zone will
         * have already routed to the approprite other bus and output will
         * be empty.
         */
        if (z->outputInfo.routeTo == DEFAULT_BUS)
        {
            if constexpr (OS)
            {
                blk::accumulate_from_to<blockSize << 1>(z->output[0], lOut);
                blk::accumulate_from_to<blockSize << 1>(z->output[1], rOut);
            }
            else
            {
                blk::accumulate_from_to<blockSize>(z->output[0], lOut);
                blk::accumulate_from_to<blockSize>(z->output[1], rOut);
            }
        }
    }

    if (rescanWeakRefs)
    {
        postZoneTraversalRemoveHandler();
    }

    // Groups are always unpitched and stereo
    auto fpitch = 0;
    bool processorConsumesMono[4]{false, false, false, false};
    bool chainIsMono{false};

    if (processors[0] || processors[1] || processors[2] || processors[3])
    {
        for (int i = 0; i < processorsPerZoneAndGroup; ++i)
        {
            if (processors[i])
            {
                memcpy(&processorIntParams[i][0], processorStorage[i].intParams.data(),
                       sizeof(processorIntParams[i]));
                processors[i]->bypassAnyway = !processorStorage[i].isActive;
            }
        }

#define CALL_ROUTE(FNN)                                                                            \
    if constexpr (OS)                                                                              \
    {                                                                                              \
        scxt::dsp::processor::FNN<OS, true>(fpitch, processors.data(), processorConsumesMono,      \
                                            processorMixOS, processorLevelOS, &endpoints,          \
                                            chainIsMono, output);                                  \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        scxt::dsp::processor::FNN<OS, true>(fpitch, processors.data(), processorConsumesMono,      \
                                            processorMix, processorLevel, &endpoints, chainIsMono, \
                                            output);                                               \
    }

        switch (outputInfo.procRouting)
        {
        case engine::HasGroupZoneProcessors<engine::Group>::procRoute_linear:
        {
            CALL_ROUTE(processSequential);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Group>::procRoute_ser2:
        {
            CALL_ROUTE(processSer2Pattern);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Group>::procRoute_ser3:
        {
            CALL_ROUTE(processSer3Pattern);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Group>::procRoute_par1:
        {
            CALL_ROUTE(processPar1Pattern);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Group>::procRoute_par2:
        {
            CALL_ROUTE(processPar2Pattern);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Group>::procRoute_par3:
        {
            CALL_ROUTE(processPar3Pattern);
        }
        break;
        case engine::HasGroupZoneProcessors<engine::Group>::procRoute_bypass:
            break;
        }
    }

    // Pan
    auto pvo = std::clamp(*endpoints.outputTarget.panP, -1.f, 1.f);

    if (pvo != 0.f)
    {
        namespace pl = sst::basic_blocks::dsp::pan_laws;
        auto pv = (std::clamp(pvo, -1.f, 1.f) + 1) * 0.5;
        pl::panmatrix_t pmat{1, 1, 0, 0};
        pl::stereoEqualPower(pv, pmat);

        for (int i = 0; i < (OS ? blockSize << 1 : blockSize); ++i)
        {
            auto il = output[0][i];
            auto ir = output[1][i];
            output[0][i] = pmat[0] * il + pmat[2] * ir;
            output[1][i] = pmat[1] * ir + pmat[3] * il;
        }
    }

    // multiply by vca level from matrix
    auto mlev = std::clamp(*endpoints.outputTarget.ampP, 0.f, 1.f);
    if constexpr (OS)
    {
        outputAmpOS.set_target(mlev * mlev * mlev);
        outputAmpOS.multiply_2_blocks(lOut, rOut);
    }
    else
    {
        outputAmp.set_target(mlev * mlev * mlev);
        outputAmp.multiply_2_blocks(lOut, rOut);
    }

    // Finally downsampleif oversmapled
    if (OS)
    {
        osDownFilter.process_block_D2(lOut, rOut, blockSize << 1);
    }
    if (activeZones == 0)
    {
        auto hasR = updateRingout() && (ringoutMax > 0) && inRingout();
        auto hasEG = hasActiveEGs();

        if (hasR || hasEG)
        {
            ringoutTime += blockSize;
        }
        else
        {
            mUILag.instantlySnap();
            parentPart->removeActiveGroup();
            ringoutMax = 0;
        }
    }
}

bool Group::updateRingout()
{
    auto res{false};
    int32_t mx{0};
    for (auto &p : processors)
    {
        if (!p)
            continue;
        auto tl = p->tail_length();
        if (tl != 0)
        {
            if (tl == -1)
            {
                /*
                 * Someone is infinite. So set ringout time to 0
                 * then if someone drops away from infinite we start
                 * counting at their max
                 */
                mx = std::numeric_limits<int32_t>::max();
                ringoutTime = 0;
            }
            else
                mx = std::max(mx, tl);
            res = true;
        }
    }
    ringoutMax = mx;
    return res;
}

void Group::addActiveZone(engine::Zone *zwp)
{
    // Add active zone to end
    activeZoneWeakRefs[activeZones] = zwp;

    if (activeZones == 0)
    {
        parentPart->addActiveGroup();
        attack();
    }
    // Important we do this *after* the attack since it allows
    // isActive to be accurate with processor ringout
    activeZones++;
    ringoutTime = 0;

    /*
    SCLOG("addZone " << SCD(activeZones));
    for (int i = 0; i < activeZones; ++i)
    {
        SCLOG("addZone " << activeZoneWeakRefs[i] << " " << activeZoneWeakRefs[i]->getName());
    }
     */
}

void Group::removeActiveZone(engine::Zone *zwp)
{
    assert(activeZones);
    rescanWeakRefs++;
}

void Group::postZoneTraversalRemoveHandler()
{
    /*
     * Go backwards down the weak refs removing inactive ones
     */
    assert(activeZones >= rescanWeakRefs);
    if (activeZones != 0)
    {
        for (int i = activeZones - 1; i >= 0; --i)
        {
            if (!activeZoneWeakRefs[i]->isActive())
            {
                rescanWeakRefs--;
                activeZoneWeakRefs[i] = activeZoneWeakRefs[activeZones - 1];
                activeZones--;
            }
        }
    }
    assert(rescanWeakRefs == 0);
    if (activeZones == 0)
    {
        ringoutMax = 0;
        ringoutTime = 0;
        updateRingout();
    }
}

engine::Engine *Group::getEngine()
{
    if (parentPart && parentPart->parentPatch)
        return parentPart->parentPatch->parentEngine;
    return nullptr;
}

const engine::Engine *Group::getEngine() const
{
    if (parentPart && parentPart->parentPatch)
        return parentPart->parentPatch->parentEngine;
    return nullptr;
}

void Group::setupOnUnstream(engine::Engine &e)
{
    onRoutingChanged();
    rePrepareAndBindGroupMatrix();

    for (auto i = 0U; i < engine::lfosPerZone; ++i)
    {
        stepLfos[i].setSampleRate(sampleRate, sampleRateInv);

        stepLfos[i].assign(&modulatorStorage[i], endpoints.lfo[i].rateP, &(getEngine()->transport),
                           getEngine()->rng);
        curveLfos[i].assign(&modulatorStorage[i], &(getEngine()->transport));
    }

    for (int p = 0; p < processorCount; ++p)
    {
        if (processors[p])
        {
            dsp::processor::unspawnProcessor(processors[p]);
            processors[p] = nullptr;
        }
        setupProcessorControlDescriptions(p, processorStorage[p].type);
        onProcessorTypeChanged(p, processorStorage[p].type);
    }

    triggerConditions.setupOnUnstream(parentPart->groupTriggerInstrumentState);
    resetPolyAndPlaymode(e);
}
void Group::onSampleRateChanged()
{
    if (getEngine())
    {
        setupOnUnstream(*getEngine());
    }

    for (auto &z : zones)
        z->setSampleRate(getSampleRate(), getSampleRateInv());

    this->setHasModulatorsSampleRate(getSampleRate(), getSampleRateInv());
}

void Group::onProcessorTypeChanged(int w, dsp::processor::ProcessorType t)
{
    if (t != dsp::processor::ProcessorType::proct_none)
    {
        if (processors[w])
        {
            dsp::processor::unspawnProcessor(processors[w]);
        }
        // FIXME - replace the float params with something modulatable
        processors[w] = dsp::processor::spawnProcessorInPlace(
            t, asT()->getEngine()->getMemoryPool().get(), processorPlacementStorage[w],
            dsp::processor::processorMemoryBufferSize, processorStorage[w],
            endpoints.processorTarget[w].fp, processorStorage[w].intParams.data(),
            outputInfo.oversample, false);

        if (processors[w])
        {
            processors[w]->setSampleRate(sampleRate * (outputInfo.oversample ? 2 : 1));
            processors[w]->setTempoPointer(&(getEngine()->transport.tempo));

            endpoints.processorTarget[w].snapValues();

            processors[w]->init();
        }
    }
    else
    {
        if (processors[w])
        {
            dsp::processor::unspawnProcessor(processors[w]);
            processors[w] = nullptr;
        }
    }
}

void Group::attack()
{
    attackInThisBlock = true;

    if (isActive())
    {
        for (int i = 0; i < egsPerGroup; ++i)
        {
            eg[i].attackFromWithDelay(eg[i].outBlock0, *(endpoints.egTarget[i].dlyP),
                                      *(endpoints.egTarget[i].aP));
        }

        // I *thin* we need to do this
        rePrepareAndBindGroupMatrix();
        return;
    }

    for (int i = 0; i < processorsPerZoneAndGroup; ++i)
    {
        const auto &ps = processorStorage[i];
        processorMix[i].set_target_instant(ps.mix);
        processorMixOS[i].set_target_instant(ps.mix);

        auto ol = ps.outputCubAmp;
        ol = ol * ol * ol * dsp::processor::ProcessorStorage::maxOutputAmp;
        processorLevel[i].set_target_instant(ol);
        processorLevelOS[i].set_target_instant(ol);
    }

    resetLFOs();
    osDownFilter.reset();
    for (int i = 0; i < egsPerGroup; ++i)
    {
        eg[i].attackFrom(0.f);
    }

    rePrepareAndBindGroupMatrix();

    // We need to re-attack with delay here since the above
    // attack set our envs to zero but now we have bound
    // enough matrix to get the delay set up.
    for (int i = 0; i < egsPerGroup; ++i)
    {
        eg[i].attackFromWithDelay(0.f, *(endpoints.egTarget[i].dlyP), *(endpoints.egTarget[i].aP));
    }
    for (int i = 0; i < processorsPerZoneAndGroup; ++i)
    {
        auto *p = processors[i];
        if (p)
        {
            endpoints.processorTarget[i].snapValues();

            p->init();
        }
    }
}

void Group::resetLFOs(int whichLFO)
{
    auto sl = (whichLFO >= 0 ? whichLFO : 0);
    auto el = (whichLFO >= 0 ? (whichLFO + 1) : engine::lfosPerZone);

    for (auto i = sl; i < el; ++i)
    {
        const auto &ms = modulatorStorage[i];

        lfoEvaluator[i] = ms.isStep() ? STEP : (ms.isEnv() ? ENV : (ms.isMSEG() ? MSEG : CURVE));

        if (lfoEvaluator[i] == STEP)
        {
            stepLfos[i].setSampleRate(sampleRate, sampleRateInv);

            stepLfos[i].assign(&modulatorStorage[i], endpoints.lfo[i].rateP,
                               &(getEngine()->transport), getEngine()->rng);
            curveLfos[i].assign(&modulatorStorage[i], &(getEngine()->transport));
        }
        else if (lfoEvaluator[i] == CURVE)
        {
            curveLfos[i].setSampleRate(sampleRate, sampleRateInv);
            curveLfos[i].attack(ms.start_phase, ms.modulatorShape);
        }
        else if (lfoEvaluator[i] == ENV)
        {
            envLfos[i].setSampleRate(sampleRate, sampleRateInv);
            envLfos[i].attack(*endpoints.lfo[i].env.delayP, *endpoints.lfo[i].env.attackP);
        }
        else
        {
            SCLOG("Unimplemented modulator shape " << ms.modulatorShape);
        }
    }

    randomEvaluator.evaluate(getEngine()->rng, miscSourceStorage);
    phasorEvaluator.attack(getEngine()->transport, miscSourceStorage);
}

bool Group::isActive() const
{
    auto haz = hasActiveZones();
    auto hae = hasActiveEGs();
    auto ir = inRingout();
    return haz || hae || ir;
}

void Group::onRoutingChanged() { rePrepareAndBindGroupMatrix(); }

void Group::resetPolyAndPlaymode(engine::Engine &e)
{
    if (outputInfo.vmPlayModeInt == (uint32_t)Engine::voiceManager_t::PlayMode::MONO_NOTES)
    {
        if (outputInfo.vmPlayModeFeaturesInt == 0)
        {
            outputInfo.vmPlayModeFeaturesInt =
                (uint64_t)Engine::voiceManager_t::MonoPlayModeFeatures::NATURAL_MONO;
        }
        auto pgrp = (uint64_t)this;
        SCLOG_IF(voiceResponder, "Setting up mono group " << pgrp);
        e.voiceManager.guaranteeGroup(pgrp);
        e.voiceManager.setPlaymode(pgrp, Engine::voiceManager_t::PlayMode::MONO_NOTES,
                                   outputInfo.vmPlayModeFeaturesInt);
    }
    else
    {
        auto pgrp = (uint64_t)this;
        SCLOG_IF(voiceResponder, "Setting up poly group " << pgrp);

        e.voiceManager.setPlaymode(pgrp, Engine::voiceManager_t::PlayMode::POLY_VOICES,
                                   outputInfo.vmPlayModeFeaturesInt);
        assert(outputInfo.vmPlayModeInt == (uint32_t)Engine::voiceManager_t::PlayMode::POLY_VOICES);
        if (!outputInfo.hasIndependentPolyLimit)
        {
            // TODO we really want a 'clear'
            e.voiceManager.setPolyphonyGroupVoiceLimit((uint64_t)this, maxVoices);
        }
        else
        {
            e.voiceManager.setPolyphonyGroupVoiceLimit((uint64_t)this, outputInfo.polyLimit);
        }
    }
}

template struct HasGroupZoneProcessors<Group>;
} // namespace scxt::engine
