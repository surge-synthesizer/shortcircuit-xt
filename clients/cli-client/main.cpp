#include <iostream>
#include <chrono>
#include <thread>

#include "engine/engine.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

struct PlaybackState
{
    uint64_t samplesPassed{0};
    int16_t prevKey{-1}, nextKey{60};
    scxt::engine::Engine *engine{nullptr};
};

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    assert(frameCount % scxt::blockSize == 0);
    auto ps = (PlaybackState *)pDevice->pUserData;

    auto engine = ps->engine;
    auto f = (float *)pOutput;

    auto fcp = ps->samplesPassed % 24000;
    ps->samplesPassed += frameCount;
    auto fnp = ps->samplesPassed % 24000;

    if (fcp == 0 || (fnp != 0 && fcp > fnp))
    {
        engine->noteOn(0, ps->nextKey, -1, 1.f, 0.f);
        if (ps->prevKey > 0)
            engine->noteOff(0, ps->prevKey, -1, 0);
        ps->prevKey = ps->nextKey;
        ps->nextKey = (ps->nextKey == 60 ? 61 : 60);
    }

    int op{0};
    for (int i = 0; i < frameCount; ++i)
    {
        if (i % scxt::blockSize == 0)
        {
            op = 0;
            engine->processAudio();
        }
        // block-wise engine process
        f[2 * i] = engine->output[0][0][op];
        f[2 * i + 1] = engine->output[0][1][op];
        op++;
    }
}

ma_device device;
bool startAudioThread(scxt::engine::Engine &e)
{
    auto pd = new PlaybackState;
    pd->engine = &e;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = 48000;
    config.dataCallback = data_callback;
    config.pUserData = pd;

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS)
    {
        return false; // Failed to initialize the device.
    }

    ma_device_start(&device);
    return true;
}
void stopAudioThread()
{
    ma_device_uninit(&device); // This will stop the device so no need to do that manually.
    auto pd = (PlaybackState *)device.pUserData;
    delete pd;
}

int main(int argc, char **argv)
{
    std::cout << "CLI Client" << std::endl;
    {
        auto engine = scxt::engine::Engine();
        for (const auto &[s, k] :
             {std::make_pair(std::string("bd1.wav"), 60), {"sd1.wav", 61}, {"wood1.wav", 60}})
        {
            auto sid = engine.getSampleManager()->loadSampleByPath(
                {"/Users/paul/Music/Samples/Korg_Minipops/" + s});

            if (!sid.has_value())
            {
                std::cout << "Unable to load sample" << std::endl;
                return 1;
            }
            auto zptr = std::make_unique<scxt::engine::Zone>(*sid);
            zptr->keyboardRange = {k, k};
            zptr->attachToSample(*engine.getSampleManager());

            engine.getPatch()->getPart(0)->getGroup(0)->addZone(zptr);
        }
        if (!startAudioThread(engine))
            return 1;

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(5s);

        stopAudioThread();
    }
    scxt::showLeakLog();
    return 0;
}
