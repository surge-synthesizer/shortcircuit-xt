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

#ifndef TEST_UTILITIES_H
#define TEST_UTILITIES_H

#include <thread>
#include <atomic>
#include "engine/engine.h"

#include "messaging/messaging.h"
#include "messaging/client/client_messages.h"

#include "console_ui.h"

namespace scxt::tests
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
            SCLOG("Engine sample rate is wrong at " << sr);
            throw std::logic_error("Set sapmle rate to something realistic");
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

struct TestHarness : scxt::MoveableOnly<TestHarness>
{
    std::unique_ptr<scxt::engine::Engine> engine;
    std::unique_ptr<ConsoleUI> editor;
    std::unique_ptr<AudioThreadProvider> audioThreadProvider;

    ~TestHarness();

    bool start(bool logMessages = false, double sampleRate = 48000);

    void stepUI(size_t forSteps = 10)
    {
        for (int i = 0; i < forSteps; ++i)
        {
            editor->stepUI();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    template <typename T> void sendToSerialization(const T &t)
    {
        scxt::messaging::client::clientSendToSerialization(t, *engine->getMessageController());
    }
};
} // namespace scxt::tests
#endif // TEST_UTILITIES_H
