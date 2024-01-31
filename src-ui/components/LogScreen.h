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

#ifndef SCXT_SRC_UI_COMPONENTS_LOGSCREEN_H
#define SCXT_SRC_UI_COMPONENTS_LOGSCREEN_H

#include "HasEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace scxt::ui
{
struct LogScreen : juce::Component, HasEditor
{
    LogScreen(SCXTEditor *e);

    std::unique_ptr<juce::TextEditor> logDisplay;
    std::unique_ptr<juce::TextButton> copyButton, closeButton;

    juce::Font displayFont;

    void visibilityChanged() override;
    void reshowLog();
    void resized() override;

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colour(0x90, 0x90, 0x90)); }

    bool keyPressed(const juce::KeyPress &key) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogScreen);
};
} // namespace scxt::ui

#endif // SHORTCIRCUIT_ABOUTESCREEN_H
