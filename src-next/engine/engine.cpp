//
// Created by Paul Walker on 1/30/23.
//

#include "engine.h"
#include "voice/voice.h"
#include "dsp/sinc_table.h"
#include "tuning/equal.h"
#include "vembertech/vt_dsp/basic_dsp.h"
#include "messaging/messaging.h"
#include "messaging/audio/audio_messages.h"

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
    messaging::MessageController::serializationToAudioMessage_t msg;
    while (messageController->serializationToAudioQueue.try_dequeue(msg))
    {
        std::cout << "GOT PROCESSOR MESSAGE" << msg << std::endl;
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
                accumulate_block(part->output[i][0], output[i][0], blockSizeQuad);
                accumulate_block(part->output[i][1], output[i][1], blockSizeQuad);
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

} // namespace scxt::engine