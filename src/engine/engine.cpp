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

#include "engine.h"
#include "part.h"
#include "voice/voice.h"
#include "dsp/sinc_table.h"
#include "tuning/equal.h"
#include "messaging/messaging.h"
#include "messaging/audio/audio_messages.h"
#include "selection/SelectionManager.h"
#include "sst/basic-blocks/mechanics/block-ops.h"

namespace scxt::engine
{

Engine::Engine()
{
    id.id = rand() % 1024;
    dsp::sincTable.init();
    tuning::equalTuning.init();

    sampleManager = std::make_unique<sample::SampleManager>();
    patch = std::make_unique<Patch>();
    patch->parentEngine = this;

    for (auto &v : voices)
        v = nullptr;

    voiceInPlaceBuffer.reset(new uint8_t[sizeof(scxt::voice::Voice) * maxVoices]);

    setStereoOutputs(1);
    selectionManager = std::make_unique<selection::SelectionManager>();

    messageController = std::make_unique<messaging::MessageController>(*this);
    messageController->start();
}

Engine::~Engine()
{
    for (auto &v : voices)
    {
        if (v)
        {
            if (v->playState != voice::Voice::OFF)
                v->release();

            v->~Voice();
            v = nullptr;
        }
    }
    messageController->stop();
}

void Engine::initiateVoice(const pathToZone_t &path)
{
    assert(zoneByPath(path));
    for (const auto &[idx, v] : sst::cpputils::enumerate(voices))
    {
        if (!v || v->playState == voice::Voice::OFF)
        {
            if (v)
            {
                voices[idx]->~Voice();
            }
            auto *dp = voiceInPlaceBuffer.get() + idx * sizeof(voice::Voice);
            const auto &z = zoneByPath(path);
            voices[idx] = new (dp) voice::Voice(z.get());

            voices[idx]->channel = path.channel;
            voices[idx]->key = path.key;
            voices[idx]->noteId = path.noteid;
            voices[idx]->setSampleRate(sampleRate, sampleRateInv);
            voices[idx]->attack();
            return;
        }
    }
}
void Engine::releaseVoice(const pathToZone_t &path)
{
    assert(zoneByPath(path));
    auto targetId = zoneByPath(path)->id;
    for (auto &v : voices)
    {
        if (v && v->playState != voice::Voice::OFF && v->zone->id == targetId && v->key == path.key)
        {
            v->release();
        }
    }
}

bool Engine::processAudio()
{
    namespace blk = sst::basic_blocks::mechanics;

    messaging::MessageController::serializationToAudioMessage_t msg;
    while (messageController->serializationToAudioQueue.try_dequeue(msg))
    {
        switch (msg.id)
        {
        case messaging::audio::s2a_dispatch_to_pointer:
        {
            auto cb =
                static_cast<messaging::MessageController::AudioThreadCallback *>(msg.payload.p);
            cb->f(*this);

            messaging::audio::AudioToSerialization rt;
            rt.id = messaging::audio::a2s_pointer_complete;
            rt.payloadType = messaging::audio::AudioToSerialization::VOID_STAR;
            rt.payload.p = (void *)cb;
            messageController->audioToSerializationQueue.try_emplace(rt);
        }
        break;
        case messaging::audio::s2a_none:
            break;
        }
    }

    // TODO This gets ripped out when voice management imporves
    auto av = activeVoiceCount();

    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));
    for (const auto &part : *patch)
    {
        if (part->isActive())
        {
            part->process();
            for (int i = 0; i < part->getNumOutputs(); ++i)
            {
                blk::accumulate_from_to<blockSize>(part->output[i][0], output[i][0]);
                blk::accumulate_from_to<blockSize>(part->output[i][1], output[i][1]);
            }
        }
    }

    auto pav = activeVoiceCount();
    if (pav != av)
    {
        messaging::audio::sendVoiceCount(pav, *messageController);
    }

    return true;
}

void Engine::noteOn(int16_t channel, int16_t key, int32_t noteId, float velocity, float detune)
{
    std::cout << "Engine::noteOn c=" << channel << " k=" << key << " nid=" << noteId
              << " v=" << velocity << std::endl;
    for (const auto &path : findZone(channel, key, noteId))
    {
        initiateVoice(path);
    }
    messaging::audio::sendVoiceCount(activeVoiceCount(), *messageController);
}
void Engine::noteOff(int16_t channel, int16_t key, int32_t noteId, float velocity)
{
    std::cout << "Engine::noteOff c=" << channel << " k=" << key << " nid=" << noteId << std::endl;

    for (const auto &path : findZone(channel, key, noteId))
    {
        releaseVoice(path);
    }
    messaging::audio::sendVoiceCount(activeVoiceCount(), *messageController);
}

uint32_t Engine::activeVoiceCount()
{
    uint32_t res{0};
    for (const auto v : voices)
    {
        if (v)
            res += (v->playState != voice::Voice::OFF);
    }
    return res;
}

const std::optional<dsp::processor::ProcessorStorage>
Engine::getProcessorStorage(const processorAddress_t &addr) const
{
    auto [part, group, zone, which] = addr;
    assert(part >= 0 && part < Patch::numParts);
    const auto &pref = getPatch()->getPart(part);

    if (!pref || group < 0)
        return {};

    const auto &gref = pref->getGroup(group);

    if (!gref || zone < 0)
        return {};

    const auto &zref = gref->getZone(zone);
    if (!zref || which < 0)
        return {};

    if (which < processorsPerZone)
        return zref->processorStorage[which];

    return {};
}

} // namespace scxt::engine