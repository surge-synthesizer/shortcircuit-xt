/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_CLIENTS_CONSOLE_UI_CONSOLE_HARNESS_H
#define SCXT_SRC_CLIENTS_CONSOLE_UI_CONSOLE_HARNESS_H

#include "console_ui.h"
#include "audio_thread_provider.h"

namespace scxt::clients::console_ui
{
struct ConsoleHarness : scxt::MoveableOnly<ConsoleHarness>
{
    std::unique_ptr<scxt::engine::Engine> engine;
    std::unique_ptr<scxt::clients::console_ui::ConsoleUI> editor;
    std::unique_ptr<scxt::clients::console_ui::AudioThreadProvider> audioThreadProvider;

    ~ConsoleHarness();

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
} // namespace scxt::clients::console_ui
#endif // SHORTCIRCUITXT_CONSOLE_HARNESS_H
