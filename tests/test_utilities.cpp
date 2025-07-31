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

#include "test_utilities.h"

namespace scxt::tests
{

bool TestHarness::start(bool logMessages, double sampleRate)
{
    engine = std::make_unique<scxt::engine::Engine>();
    engine->runningEnvironment = "SCXT Test Harness";

    engine->setSampleRate(sampleRate);

    editor = std::make_unique<ConsoleUI>(*(engine->getMessageController()), *(engine->defaults),
                                         *(engine->getSampleManager()), *(engine->getBrowser()),
                                         engine->sharedUIMemoryState);
    editor->logMessages = logMessages;

    audioThreadProvider = std::make_unique<AudioThreadProvider>(*engine);
    return true;
}

TestHarness::~TestHarness()
{
    audioThreadProvider.reset();
    editor.reset();
    engine.reset();
}
} // namespace scxt::tests