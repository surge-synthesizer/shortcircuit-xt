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

#include "infrastructure/sse_include.h"

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
      modulation::shared::HasModulators<Group>(this), osDownFilter(6, true)
{
}

void Group::rePrepareAndBindGroupMatrix()
{
    endpoints.sources.bind(modMatrix, *this);
    modMatrix.prepare(routingTable);
    endpoints.bindTargetBaseValues(modMatrix, *this);
    modMatrix.process();
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

    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));

    // When we have alternate group routing we change these pointers
    float *lOut = output[0];
    float *rOut = output[1];

    bool gated{false};
    for (const auto &z : zones)
    {
        gated = gated || (z->gatedVoiceCount > 0);
    }

    for (auto i = 0; i < engine::lfosPerZone; ++i)
    {
        if (lfoEvaluator[i] == STEP)
        {
            stepLfos[i].process(blockSize);
        }
        else if (lfoEvaluator[i] == CURVE)
        {
            auto &lp = endpoints.lfo[i];

            curveLfos[i].process(*lp.rateP, *lp.curve.deformP, *lp.curve.delayP, *lp.curve.attackP,
                                 *lp.curve.releaseP, modulatorStorage[i].curveLfoStorage.useenv,
                                 modulatorStorage[i].curveLfoStorage.unipolar, gated);
        }
        else if (lfoEvaluator[i] == ENV)
        {
            auto &lp = endpoints.lfo[i].env;

            envLfos[i].process(*lp.delayP, *lp.attackP, *lp.holdP, *lp.decayP, *lp.sustainP,
                               *lp.releaseP, gated);
        }
        else
        {
        }
    }

    bool envGate = gated;
    auto &aegp = endpoints.eg[0];
    eg[0].processBlock(*aegp.aP, *aegp.hP, *aegp.dP, *aegp.sP, *aegp.rP, *aegp.asP, *aegp.dsP,
                       *aegp.rsP, envGate);
    auto &eg2p = endpoints.eg[1];
    eg[1].processBlock(*eg2p.aP, *eg2p.hP, *eg2p.dP, *eg2p.sP, *eg2p.rP, *eg2p.asP, *eg2p.dsP,
                       *eg2p.rsP, envGate);

    modMatrix.process();

    for (const auto &z : zones)
    {
        if (z->isActive())
        {
            z->process(e);
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
    }

    // Groups are always unpitched and stereo
    auto fpitch = 0;
    bool processorConsumesMono[4]{false, false, false, false};
    bool chainIsMono{false};

    if (processors[0] || processors[1] || processors[2] || processors[3])
    {

#define CALL_ROUTE(FNN)                                                                            \
    if constexpr (OS)                                                                              \
    {                                                                                              \
        scxt::dsp::processor::FNN<OS, true>(fpitch, processors.data(), processorConsumesMono,      \
                                            processorMixOS, &endpoints, chainIsMono, output);      \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
        scxt::dsp::processor::FNN<OS, true>(fpitch, processors.data(), processorConsumesMono,      \
                                            processorMix, &endpoints, chainIsMono, output);        \
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
}

void Group::addActiveZone()
{
    if (activeZones == 0)
    {
        parentPart->addActiveGroup();
        attack();
    }
    activeZones++;
}

void Group::removeActiveZone()
{
    assert(activeZones);
    activeZones--;
    if (activeZones == 0)
    {
        mUILag.instantlySnap();
        parentPart->removeActiveGroup();
    }
}

engine::Engine *Group::getEngine()
{
    if (parentPart && parentPart->parentPatch)
        return parentPart->parentPatch->parentEngine;
    return nullptr;
}

void Group::setupOnUnstream(const engine::Engine &e)
{
    rePrepareAndBindGroupMatrix();

    for (auto i = 0U; i < engine::lfosPerZone; ++i)
    {
        stepLfos[i].setSampleRate(sampleRate, sampleRateInv);

        stepLfos[i].assign(&modulatorStorage[i], endpoints.lfo[i].rateP, nullptr,
                           getEngine()->rngGen);
    }

    for (int p = 0; p < processorCount; ++p)
    {
        setupProcessorControlDescriptions(p, processorStorage[p].type);
        onProcessorTypeChanged(p, processorStorage[p].type);
    }

    for (auto &lfo : stepLfos)
    {
        lfo.UpdatePhaseIncrement();
    }
}
void Group::onSampleRateChanged()
{
    if (getEngine())
    {
        setupOnUnstream(*getEngine());
    }

    for (auto &z : zones)
        z->setSampleRate(getSampleRate(), getSampleRateInv());
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
    for (int i = 0; i < processorsPerZoneAndGroup; ++i)
    {
        const auto &ps = processorStorage[i];
        processorMix[i].set_target_instant(ps.mix);
        processorMixOS[i].set_target_instant(ps.mix);
    }

    osDownFilter.reset();
    resetLFOs();
    rePrepareAndBindGroupMatrix();
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

            stepLfos[i].assign(&modulatorStorage[i], endpoints.lfo[i].rateP, nullptr,
                               getEngine()->rngGen);
        }
        else if (lfoEvaluator[i] == CURVE)
        {
            curveLfos[i].setSampleRate(sampleRate, sampleRateInv);
            curveLfos[i].attack(ms.start_phase, ms.modulatorShape);
        }
        else if (lfoEvaluator[i] == ENV)
        {
            envLfos[i].setSampleRate(sampleRate, sampleRateInv);
            envLfos[i].attack(*endpoints.lfo[i].env.delayP);
        }
        else
        {
            SCLOG("Unimplemented modulator shape " << ms.modulatorShape);
        }
    }
}

template struct HasGroupZoneProcessors<Group>;
} // namespace scxt::engine