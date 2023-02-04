//
// Created by Paul Walker on 1/30/23.
//

#include "app.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

namespace scxt::juce_app
{

ma_device device;

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    auto ps = (PlaybackState *)pDevice->pUserData;

    auto engine = ps->engine;
    auto f = (float *)pOutput;

    auto fcp = ps->samplesPassed % 44000;
    ps->samplesPassed += frameCount;
    auto fnp = ps->samplesPassed % 44000;

    if (ps->autoPlay && (fcp == 0 || (fnp != 0 && fcp > fnp)))
    {
        engine->noteOn(0, ps->nextKey, -1, 1.f, 0.f);
        if (ps->prevKey > 0)
            engine->noteOff(0, ps->prevKey, -1, 0);
        ps->prevKey = ps->nextKey;
        ps->nextKey = (ps->nextKey == 60 ? 61 : 60);
    }

    while (ps->readPoint < ps->writePoint)
    {
        auto ri = ps->readPoint & (PlaybackState::maxMsg - 1);

        auto *ms = ps->msgs[ri];
        if (ms[0] == 0x90)
        {
            engine->noteOn(0, ms[1], -1, ms[2] / 127.f, 0.f);
        }
        if (ms[0] == 0x80)
        {
            engine->noteOff(0, ms[1], -1, ms[2] / 127.f);
        }
        ps->readPoint++;
    }

    for (int i = 0; i < frameCount; ++i)
    {
        if (ps->outpos == scxt::blockSize)
        {
            ps->outpos = 0;
            engine->processAudio();
        }
        // block-wise engine process
        f[2 * i] = engine->output[0][0][ps->outpos];
        f[2 * i + 1] = engine->output[0][1][ps->outpos];
        ps->outpos++;
    }
}
bool startAudioThread(PlaybackState *pd, bool autoPlay)
{
    pd->autoPlay = autoPlay;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    // config.sampleRate = 44100;
    config.sampleRate = 48000;
    config.dataCallback = data_callback;
    config.pUserData = pd;

    pd->engine->setSampleRate(config.sampleRate);

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS)
    {
        return false; // Failed to initialize the device.
    }

    ma_device_start(&device);
    return true;
}
bool stopAudioThread(PlaybackState *s)
{
    ma_device_uninit(&device); // This will stop the device so no need to do that manually.
    return true;
}
} // namespace scxt::cli_client