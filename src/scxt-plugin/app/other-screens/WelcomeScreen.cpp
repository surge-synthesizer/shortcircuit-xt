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

#include "WelcomeScreen.h"

#include "utils.h"
#include "app/SCXTEditor.h"
#include "infrastructure/user_defaults.h"

namespace scxt::ui::app::other_screens
{
WelcomeScreen::WelcomeScreen(SCXTEditor *e) : HasEditor(e)
{
    setAccessible(true);
    setTitle("Welcome to ShortCircuit XT Beta. Please use care. Press escape to use sampler.");
    setWantsKeyboardFocus(true);
}
void WelcomeScreen::visibilityChanged()
{
    if (isVisible())
        grabKeyboardFocus();
}
bool WelcomeScreen::keyPressed(const juce::KeyPress &key)
{
    if (key.getKeyCode() == juce::KeyPress::escapeKey)
    {
        okGotItDontShowAgain();
        return true;
    }
    return false;
}

void WelcomeScreen::okGotItDontShowAgain()
{
    editor->defaultsProvider.updateUserDefaultValue(infrastructure::welcomeScreenSeen,
                                                    welcomeVersion);
    setVisible(false);
}

auto txt =
    "Welcome to the Shortcircuit XT beta! The synth is now stable, many of the "
    "features are complete, and we are opening up the tool to a wider audience for "
    "testing and feedback as we push towards 1.0 in 2026.\n"
    "\n"
    "As with all beta software, we strongly suggest you consider using limiter and "
    "don't use in-ear headphones when experimenting.\n"
    "\n"
    "We love early testers, documenters, and designers on all our projects. The best way to "
    "chat with the Surge Synth Team devs about ShortcircuitXT is to join the Surge Synth Team "
    "discord and hop into the #using-shortcircuit or #sc-development channels. There is a link "
    "to our discord in the synth about screen, available by pressing the logo in the upper right "
    "corner\n\n"
    "Finally, we love developers too! If you want to join the team and help sling some code on the "
    "project, please get in touch.\n\n"
    "Press escape or click on this screen to dismiss. If you want to re-review this information, "
    "choose 'Show Welcome Screen' from the menu in the upper right corner.";
;

void WelcomeScreen::paint(juce::Graphics &g)
{
    auto r = getLocalBounds();

    g.fillAll(editor->themeColor(theme::ColorMap::bg_1).withAlpha(0.6f));

    auto bd = r.reduced(140, 120);

    g.setColour(editor->themeColor(theme::ColorMap::bg_1));
    g.fillRect(bd);
    g.setColour(editor->themeColor(theme::ColorMap::generic_content_medium));
    g.drawRect(bd);
    g.setFont(editor->themeApplier.interBoldFor(40));
    g.setColour(editor->themeColor(theme::ColorMap::generic_content_highest));
    g.drawText("Welcome to ShortcircuitXT", bd.reduced(10), juce::Justification::centredTop);
    g.setColour(editor->themeColor(theme::ColorMap::warning_1a));
    g.drawText("Beta Release", bd.reduced(10).translated(0, 50), juce::Justification::centredTop);

    g.setFont(editor->themeApplier.interLightFor(22));
    g.setColour(editor->themeColor(theme::ColorMap::generic_content_high));
    auto tb = bd.reduced(10, 120);
    g.drawFittedText(txt, tb, juce::Justification::topLeft, 50);
}
} // namespace scxt::ui::app::other_screens