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
