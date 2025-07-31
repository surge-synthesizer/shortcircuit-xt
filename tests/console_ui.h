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

#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include <memory>
#include <thread>
#include <atomic>

#include "utils.h"
#include "engine/engine.h"
#include "messaging/client/selection_messages.h"
#include "messaging/messaging.h"
#include "selection/selection_manager.h"

namespace scxt::tests
{
struct ConsoleUI
{
    messaging::MessageController &msgCont;
    const sample::SampleManager &sampleManager;
    infrastructure::DefaultsProvider &defaultsProvider;
    const scxt::browser::Browser &browser;
    const engine::Engine::SharedUIMemoryState &sharedUiMemoryState;

    ConsoleUI(messaging::MessageController &e, infrastructure::DefaultsProvider &d,
              const sample::SampleManager &s, const scxt::browser::Browser &b,
              const engine::Engine::SharedUIMemoryState &state);
    ~ConsoleUI();
};

struct TestHarness : scxt::MoveableOnly<TestHarness>
{
    std::unique_ptr<scxt::engine::Engine> engine;
    std::unique_ptr<ConsoleUI> editor;

    bool start(double sampleRate = 48000);

    ~TestHarness();

    // to do non copyable

  private:
    std::unique_ptr<std::thread> audioThread;
    std::atomic<bool> keepProcessing;
    void runAudioThread();
};
} // namespace scxt::tests

#endif // CONSOLE_UI_H
