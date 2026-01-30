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

#include "LogScreen.h"

#include "utils.h"
#include "sst/plugininfra/cpufeatures.h"
#include "app/SCXTEditor.h"
#include "connectors/SCXTResources.h"

namespace scxt::ui::app::other_screens
{
LogScreen::LogScreen(SCXTEditor *e) : HasEditor(e)
{
    auto interMed =
        connectors::resources::loadTypeface("fonts/Anonymous_Pro/AnonymousPro-Regular.ttf");

    displayFont = editor->themeApplier.anonmyousProRegularFor(10);

    logDisplay = std::make_unique<juce::TextEditor>();
    logDisplay->setMultiLine(true);
    logDisplay->setReadOnly(true);
    logDisplay->setFont(displayFont);
    addAndMakeVisible(*logDisplay);

    closeButton = std::make_unique<juce::TextButton>();
    closeButton->setTitle("Close");
    closeButton->setButtonText("Close");
    closeButton->onClick = [w = juce::Component::SafePointer(this)]() {
        if (w)
            w->setVisible(false);
    };
    addAndMakeVisible(*closeButton);
}

void LogScreen::visibilityChanged()
{
    if (isVisible())
    {
        reshowLog();
        logDisplay->grabKeyboardFocus();
    }
}

void LogScreen::reshowLog() { logDisplay->setText(scxt::getFullLog(), juce::dontSendNotification); }

void LogScreen::resized()
{
    auto bh = 40;
    auto tb = getLocalBounds().withTrimmedBottom(bh);
    auto bb = getLocalBounds().withTop(getHeight() - bh);

    logDisplay->setBounds(tb.reduced(3));
    closeButton->setBounds(bb.withLeft(bb.getRight() - 120).reduced(3));
}
bool LogScreen::keyPressed(const juce::KeyPress &key)
{
    if (key.getKeyCode() == juce::KeyPress::escapeKey)
    {
        setVisible(false);
        return true;
    }
    return false;
}
} // namespace scxt::ui::app::other_screens