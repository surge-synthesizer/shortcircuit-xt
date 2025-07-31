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

#include <chrono>
#include "console_ui.h"

namespace scxt::tests
{

ConsoleUI::ConsoleUI(messaging::MessageController &e, infrastructure::DefaultsProvider &d,
                     const sample::SampleManager &s, const scxt::browser::Browser &b,
                     const engine::Engine::SharedUIMemoryState &st)
    : msgCont(e), sampleManager(s), defaultsProvider(d), browser(b), sharedUiMemoryState(st)
{
    msgCont.registerClient("SCXTEditor", [this](auto &s) {
        SCLOG("Test Harness message recieved " << s.size() << " bytes");
        {
            // Remember this runs on the serialization thread so needs to be thread safe
            // std::lock_guard<std::mutex> g(callbackMutex);
            // callbackQueue.push(s);
        }
        // juce::MessageManager::callAsync([this]() { drainCallbackQueue(); });
    });
}

ConsoleUI::~ConsoleUI() { msgCont.unregisterClient(); }

bool TestHarness::start(double sampleRate)
{
    engine = std::make_unique<scxt::engine::Engine>();
    engine->runningEnvironment = "SCXT Test Harness";

    engine->setSampleRate(sampleRate);

    editor = std::make_unique<ConsoleUI>(*(engine->getMessageController()), *(engine->defaults),
                                         *(engine->getSampleManager()), *(engine->getBrowser()),
                                         engine->sharedUIMemoryState);

    keepProcessing = true;
    audioThread = std::make_unique<std::thread>([this]() { runAudioThread(); });
    return true;
}

TestHarness::~TestHarness()
{
    keepProcessing.store(false);
    audioThread->join();
    editor.reset();
    engine.reset();
}

void TestHarness::runAudioThread()
{
    auto sr = engine->getSampleRate();
    auto sleepTime = (uint32_t)(scxt::blockSize / sr * 1000000 * 0.5);

    while (keepProcessing)
    {
        engine->processAudio();
        engine->transport.timeInBeats +=
            (double)scxt::blockSize * 120 * engine->getSampleRateInv() / 60.f;
        std::this_thread::sleep_for(std::chrono::microseconds(sleepTime));
    }
}

} // namespace scxt::tests