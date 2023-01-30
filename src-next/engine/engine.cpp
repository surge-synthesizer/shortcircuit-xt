//
// Created by Paul Walker on 1/30/23.
//

#include "engine.h"
#include "voice/voice.h"

namespace scxt::engine
{

Engine::Engine()
{
    sampleManager = std::make_unique<sample::SampleManager>();
    patch = std::make_unique<Patch>();

    for (auto &v : voices)
        v = nullptr;

    voiceInPlaceBuffer.reset(new uint8_t[sizeof(scxt::voice::Voice) * maxVoices]);

    setStereoOutputs(1);
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
        }
    }
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
            voices[idx] = new (dp) voice::Voice(*(zoneByPath(path)));
            voices[idx]->key = path.key;
            voices[idx]->sampleRate = sampleRate;
            voices[idx]->sampleRateInv = sampleRateInv;
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
        if (v && v->playState != voice::Voice::OFF &&
            v->zone.id == targetId && v->key == path.key)
        {
            v->release();
        }
    }
}

bool Engine::processAudio()
{
    // TODO This is stereo only now
    memset(output[0], 0, sizeof(output[0]));
    for (auto &v : voices)
    {
        if (v && v->playState != voice::Voice::OFF)
        {
            if (v->process())
            {
                // TODO SIMDIZE
                for (int c = 0; c < 2; ++c)
                    for (int s = 0; s < blockSize; ++s)
                        output[0][c][s] += v->output[c][s];
            }
        }
    }
    return true;
}
} // namespace scxt::engine