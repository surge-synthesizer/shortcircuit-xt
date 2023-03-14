/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#include "zone.h"
#include "group.h"
#include "part.h"
#include "engine.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "sst/basic-blocks/mechanics/block-ops.h"

namespace scxt::engine
{
void Zone::process()
{
    namespace blk = sst::basic_blocks::mechanics;
    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));

    std::array<voice::Voice *, maxVoices> toCleanUp;
    size_t cleanupIdx{0};
    for (auto &v : voiceWeakPointers)
    {
        if (v && v->playState != voice::Voice::OFF)
        {
            if (v->process())
            {
                blk::accumulate_from_to<blockSize>(v->output[0], output[0][0]);
                blk::accumulate_from_to<blockSize>(v->output[1], output[0][1]);
            }
            if (v->playState == voice::Voice::CLEANUP)
            {
                toCleanUp[cleanupIdx++] = v;
            }
        }
    }
    for (int i = 0; i < cleanupIdx; ++i)
    {
        toCleanUp[i]->cleanupVoice();
    }
}

void Zone::addVoice(voice::Voice *v)
{
    if (activeVoices == 0)
    {
        parentGroup->addActiveZone();
    }
    activeVoices++;
    for (auto &nv : voiceWeakPointers)
    {
        if (nv == nullptr)
        {
            nv = v;
            return;
        }
    }
    assert(false);
}
void Zone::removeVoice(voice::Voice *v)
{
    for (auto &nv : voiceWeakPointers)
    {
        if (nv == v)
        {
            nv = nullptr;
            activeVoices--;
            if (activeVoices == 0)
            {
                parentGroup->removeActiveZone();
            }
            return;
        }
    }
    assert(false);
}

engine::Engine *Zone::getEngine()
{
    if (parentGroup && parentGroup->parentPart && parentGroup->parentPart->parentPatch)
        return parentGroup->parentPart->parentPatch->parentEngine;
    return nullptr;
}

void Zone::setProcessorType(int whichProcessor, dsp::processor::ProcessorType type)
{
    assert(getEngine() && getEngine()->getMessageController()->threadingChecker.isAudioThread());

    auto &ps = processorStorage[whichProcessor];
    ps.type = type;
    ps.mix = 1.0;

    uint8_t memory[dsp::processor::processorMemoryBufferSize];
    dsp::processor::Processor *tmpProcessor{nullptr};
    float pfp[dsp::processor::maxProcessorFloatParams];
    int ifp[dsp::processor::maxProcessorIntParams];

    tmpProcessor =
        dsp::processor::spawnProcessorInPlace(type, getEngine()->getMemoryPool().get(), memory,
                                              dsp::processor::processorMemoryBufferSize, pfp, ifp);

    if (type != dsp::processor::proct_none)
    {
        assert(tmpProcessor);
        assert(getEngine());
        tmpProcessor->setSampleRate(getEngine()->getSampleRate());
        tmpProcessor->init();
        tmpProcessor->init_params();

        memcpy(&(ps.floatParams[0]), pfp, sizeof(ps.floatParams));
        memcpy(&(ps.intParams[0]), ifp, sizeof(ps.intParams));
        setupProcessorControlDescriptions(whichProcessor, type, tmpProcessor);

        // TODO: The control descriptions get populated now also
        dsp::processor::unspawnProcessor(tmpProcessor);
    }
    else
    {
        assert(!tmpProcessor);
        setupProcessorControlDescriptions(whichProcessor, type, tmpProcessor);
    }
}

void Zone::initialize()
{
    for (auto &v : voiceWeakPointers)
        v = nullptr;

    for (auto &l : lfoStorage)
    {
        modulation::modulators::load_lfo_preset(modulation::modulators::lp_clear, &l);
    }
}

void Zone::setupProcessorControlDescriptions(int whichProcessor, dsp::processor::ProcessorType type,
                                             dsp::processor::Processor *tmpProcessorFromAfar)
{
    if (type == dsp::processor::proct_none)
    {
        processorDescription[whichProcessor] = {};
        processorDescription[whichProcessor].typeDisplayName = "Off";
        return;
    }

    auto *tmpProcessor = tmpProcessorFromAfar;

    assert(getEngine() && getEngine()->getMessageController()->threadingChecker.isAudioThread());

    uint8_t memory[dsp::processor::processorMemoryBufferSize];
    float pfp[dsp::processor::maxProcessorFloatParams];
    int ifp[dsp::processor::maxProcessorIntParams];

    if (!tmpProcessor)
    {
        tmpProcessor = dsp::processor::spawnProcessorInPlace(
            type, getEngine()->getMemoryPool().get(), memory,
            dsp::processor::processorMemoryBufferSize, pfp, ifp);
    }

    assert(tmpProcessor);

    processorDescription[whichProcessor] = tmpProcessor->getControlDescription();

    if (!tmpProcessorFromAfar)
        dsp::processor::unspawnProcessor(tmpProcessor);
}

void Zone::setupOnUnstream(const engine::Engine &e)
{
    attachToSample(*(e.getSampleManager()));
    for (int p = 0; p < processorsPerZone; ++p)
    {
        setupProcessorControlDescriptions(p, processorStorage[p].type);
    }
}

std::string Zone::toStreamingNamePlayModes(PlayModes p)
{
    switch (p)
    {
    case STANDARD:
        return "standard";
    case LOOP:
        return "loop";
    case LOOP_UNTIL_RELEASE:
        return "loopuntilrelease";
    case LOOP_BIDIRECTIONAL:
        return "loopbidirectional";
    case ONESHOT:
        return "oneshot";
    case ONRELEASE:
        return "onrelease";
        // case SLICED_KEYMAP:
        //     return "slicedkeymap";
    }
    throw std::logic_error("Bad Playmode");
}

Zone::PlayModes Zone::fromStreamingNamePlayModes(const std::string &s)
{
    if (s == "standard")
        return STANDARD;
    if (s == "loop")
        return LOOP;
    if (s == "loopuntilrelease")
        return LOOP_UNTIL_RELEASE;
    if (s == "loopbidirectional")
        return LOOP_BIDIRECTIONAL;
    if (s == "oneshot")
        return ONESHOT;
    if (s == "onrelease")
        return ONRELEASE;
    // if (s == "slicedkeymap")
    //     return SLICED_KEYMAP;

    return STANDARD;
}

} // namespace scxt::engine