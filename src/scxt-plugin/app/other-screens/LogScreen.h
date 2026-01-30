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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_OTHER_SCREENS_LOGSCREEN_H
#define SCXT_SRC_SCXT_PLUGIN_APP_OTHER_SCREENS_LOGSCREEN_H

#include "app/HasEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace scxt::ui::app::other_screens
{
struct LogScreen : juce::Component, HasEditor
{
    LogScreen(SCXTEditor *e);

    std::unique_ptr<juce::TextEditor> logDisplay;
    std::unique_ptr<juce::TextButton> copyButton, closeButton;

#if JUCE_VERSION >= 0x080000
    juce::Font displayFont{juce::FontOptions(1)};
#else
    juce::Font displayFont{};
#endif

    void visibilityChanged() override;
    void reshowLog();
    void resized() override;

    void paint(juce::Graphics &g) override { g.fillAll(juce::Colour(0x90, 0x90, 0x90)); }

    bool keyPressed(const juce::KeyPress &key) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogScreen);
};
} // namespace scxt::ui::app::other_screens

#endif // SHORTCIRCUIT_ABOUTESCREEN_H
