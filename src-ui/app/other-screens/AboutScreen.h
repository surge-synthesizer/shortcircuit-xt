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

#ifndef SCXT_SRC_UI_APP_OTHER_SCREENS_ABOUTSCREEN_H
#define SCXT_SRC_UI_APP_OTHER_SCREENS_ABOUTSCREEN_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/TextPushButton.h"
#include "app/HasEditor.h"

namespace scxt::ui::app::other_screens
{
struct AboutLink;
struct AboutScreen : juce::Component, HasEditor
{
    struct AboutInfo
    {
        std::string title;
        std::string value;
        bool isBig{true};
    };
    std::vector<AboutInfo> info;

    explicit AboutScreen(SCXTEditor *e);
    ~AboutScreen();
    void paint(juce::Graphics &g) override;

    std::unique_ptr<juce::Drawable> icon;
    std::unique_ptr<sst::jucegui::components::TextPushButton> copyButton;

#if JUCE_VERSION >= 0x080000
#define FT(x) x{juce::FontOptions(1)}
#else
#define FT(x)                                                                                      \
    x {}
#endif
    juce::Font FT(titleFont), FT(subtitleFont), FT(infoFont), FT(aboutFont);
#undef FT

    void resetInfo();
    void copyInfo();
    void resized() override;

    void visibilityChanged() override;

    void mouseUp(const juce::MouseEvent &e) override { setVisible(false); }

    static constexpr int infoh = 23;

    static constexpr int openSourceStartY{210}, openSourceX{25};
    std::string openSourceText;
    std::vector<std::pair<std::string, std::string>> openSourceInfo;
    std::vector<std::unique_ptr<AboutLink>> openSourceLinkWidgets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutScreen);
};
} // namespace scxt::ui::app::other_screens

#endif // SCXT_SRC_UI_COMPONENTS_ABOUTSCREEN_H
