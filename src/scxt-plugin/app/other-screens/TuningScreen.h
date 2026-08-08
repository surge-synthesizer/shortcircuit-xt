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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_OTHER_SCREENS_TUNINGSCREEN_H
#define SCXT_SRC_SCXT_PLUGIN_APP_OTHER_SCREENS_TUNINGSCREEN_H

#include "app/HasEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <string>
#include "sst/jucegui/components/NamedPanel.h"

namespace scxt::ui::app::other_screens
{
struct TuningScreenContents;

/*
 * Shows the SCL scale and KBM mapping as editable monospace text. Deliberately
 * a plain text box for now; a structured scale editor can come later.
 */
struct TuningScreen : juce::Component, HasEditor
{
    TuningScreen(SCXTEditor *e);
    ~TuningScreen();

    // Called from the s2c handler with whatever the engine currently holds
    void setSclKbmFromEngine(const std::string &scl, const std::string &kbm);

    void doLoad();
    void doApply();
    void doRevert();
    void setActiveTab(int);

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colour(0x90, 0x90, 0x90).withAlpha(0.3f));
    }
    void resized() override;
    void visibilityChanged() override;
    bool keyPressed(const juce::KeyPress &key) override;

    std::unique_ptr<sst::jucegui::components::NamedPanel> contentsArea;
    std::unique_ptr<juce::FileChooser> fileChooser;
    int activeTab{0};

  private:
    TuningScreenContents *contents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TuningScreen);
};
} // namespace scxt::ui::app::other_screens

#endif // SCXT_SRC_SCXT_PLUGIN_APP_OTHER_SCREENS_TUNINGSCREEN_H
