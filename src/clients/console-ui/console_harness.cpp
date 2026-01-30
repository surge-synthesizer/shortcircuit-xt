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

#include "console_harness.h"

namespace scxt::clients::console_ui
{

bool ConsoleHarness::start(bool logMessages, double sampleRate)
{
    engine = std::make_unique<scxt::engine::Engine>();
    engine->runningEnvironment = "SCXT Console Harness";

    engine->setSampleRate(sampleRate);

    editor = std::make_unique<scxt::clients::console_ui::ConsoleUI>(
        *(engine->getMessageController()), *(engine->defaults), *(engine->getSampleManager()),
        *(engine->getBrowser()), engine->sharedUIMemoryState);
    editor->logMessages = logMessages;

    audioThreadProvider = std::make_unique<scxt::clients::console_ui::AudioThreadProvider>(*engine);
    return true;
}

ConsoleHarness::~ConsoleHarness()
{
    audioThreadProvider.reset();
    editor.reset();
    engine.reset();
}
} // namespace scxt::clients::console_ui