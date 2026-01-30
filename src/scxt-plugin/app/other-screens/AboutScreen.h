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

#ifndef SCXT_SRC_SCXT_PLUGIN_APP_OTHER_SCREENS_ABOUTSCREEN_H
#define SCXT_SRC_SCXT_PLUGIN_APP_OTHER_SCREENS_ABOUTSCREEN_H

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
    std::unique_ptr<juce::Drawable> aboutLinkIcons;
    std::unique_ptr<AboutLink> copyButton;

    static constexpr int numButtons{8};
    static constexpr int buttonSize{50};
    static constexpr int iconSize{36};

    juce::Rectangle<int> buttonRects[numButtons];

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

    void mouseUp(const juce::MouseEvent &e) override
    {
        for (int i = 0; i < numButtons; ++i)
        {
            if (buttonRects[i].toFloat().contains(e.position))
            {
                buttonRectActions[i]();
                return;
            }
        }
        setVisible(false);
    }

    std::vector<std::function<void()>> buttonRectActions{
        []() {
            juce::URL("https://github.com/surge-synthesizer/shortcircuit-xt")
                .launchInDefaultBrowser();
        },
        []() { juce::URL("https://discord.gg/aFQDdMV").launchInDefaultBrowser(); },
        []() {
            juce::URL("https://www.gnu.org/licenses/gpl-3.0-standalone.html")
                .launchInDefaultBrowser();
        },
        []() { juce::URL("https://cleveraudio.org").launchInDefaultBrowser(); },
        []() {
            juce::URL("https://developer.apple.com/documentation/audiounit")
                .launchInDefaultBrowser();
        },
        []() { juce::URL("https://www.steinberg.net/technology/").launchInDefaultBrowser(); },
        []() { juce::URL("https://www.steinberg.net/technology/").launchInDefaultBrowser(); },
        []() { juce::URL("https://juce.com").launchInDefaultBrowser(); }};

    void mouseEnter(const juce::MouseEvent &event) override
    {
        mpos = event.position;
        repaint();
    }

    void mouseMove(const juce::MouseEvent &event) override
    {
        mpos = event.position;
        repaint();
    }
    juce::Point<float> mpos;

    static constexpr int infoh = 23;

    static constexpr int openSourceStartY{30}, openSourceX{10};
    std::string openSourceText;
    std::vector<std::pair<std::string, std::string>> openSourceInfo;
    std::vector<std::unique_ptr<AboutLink>> openSourceLinkWidgets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutScreen);
};
} // namespace scxt::ui::app::other_screens

#endif // SCXT_SRC_UI_COMPONENTS_ABOUTSCREEN_H
