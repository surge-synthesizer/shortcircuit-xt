/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_CLIENTS_CONSOLE_UI_AUDIO_THREAD_PROVIDER_H
#define SCXT_SRC_CLIENTS_CONSOLE_UI_AUDIO_THREAD_PROVIDER_H

namespace scxt::clients::console_ui
{
struct AudioThreadProvider
{
    AudioThreadProvider(engine::Engine &e) : engine(e)
    {
        keepProcessing = true;
        audioThread = std::make_unique<std::thread>([this]() { runAudioThread(); });
    }
    ~AudioThreadProvider()
    {
        if (keepProcessing)
        {
            keepProcessing.store(false);
            audioThread->join();
        }
    }

  private:
    engine::Engine &engine;
    std::unique_ptr<std::thread> audioThread;
    std::atomic<bool> keepProcessing{false};
    void runAudioThread()
    {
        auto sr = engine.getSampleRate();
        if (sr < 44100)
        {
            SCLOG_IF(warnings, "Engine sample rate is wrong at " << sr);
            throw std::logic_error("Set sample rate to something realistic");
        }
        auto sleepTime = (uint32_t)(scxt::blockSize / sr * 1000000 * 0.5);

        while (keepProcessing)
        {
            engine.processAudio();
            engine.transport.timeInBeats +=
                (double)scxt::blockSize * 120 * engine.getSampleRateInv() / 60.f;
            std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
        }
    }
};
} // namespace scxt::clients::console_ui
#endif // SHORTCIRCUITXT_AUDIO_THREAD_PROVIDER_H
