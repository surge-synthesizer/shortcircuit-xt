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

#include "AboutScreen.h"
#include "utils.h"
#include "sst/plugininfra/version_information.h"
#include "sst/plugininfra/cpufeatures.h"
#include "app/SCXTEditor.h"
#include "connectors/SCXTResources.h"

namespace scxt::ui::app::other_screens
{
struct AboutLink : juce::Component, HasEditor
{
    std::string url, txt;
    AboutLink(SCXTEditor *e, const std::string &url, const std::string tx)
        : HasEditor(e), url(url), txt(tx)
    {
        font = editor->themeApplier.interLightFor(11);
    }
    bool isHovered{false};
    void mouseEnter(const juce::MouseEvent &event) override
    {
        isHovered = true;
        repaint();
    }
    void mouseExit(const juce::MouseEvent &event) override
    {
        isHovered = false;
        repaint();
    }
    void mouseDown(const juce::MouseEvent &event) override
    {
        if (onClick)
        {
            onClick();
            return;
        }

        juce::URL(url).launchInDefaultBrowser();
    }
    void paint(juce::Graphics &g) override
    {
        if (isHovered)
        {
            g.setColour(editor->themeColor(hoverColor));
        }
        else
        {
            g.setColour(editor->themeColor(displayColor));
        }
        g.setFont(font);
        g.drawText(txt, getLocalBounds(), juce::Justification::centredLeft);
    }

    juce::Font font{juce::FontOptions()};
    theme::ColorMap::Colors hoverColor{theme::ColorMap::accent_2a},
        displayColor{theme::ColorMap::generic_content_medium};
    std::function<void()> onClick{nullptr};
};

AboutScreen::AboutScreen(SCXTEditor *e) : HasEditor(e)
{
    icon = connectors::resources::loadImageDrawable("images/SCAppIcon.svg");
    aboutLinkIcons = connectors::resources::loadImageDrawable("images/about-icons.svg");
    titleFont = editor->themeApplier.interMediumFor(70);
    subtitleFont = editor->themeApplier.interMediumFor(20);
    infoFont = editor->themeApplier.interMediumFor(12);
    aboutFont = editor->themeApplier.interMediumFor(18);

    copyButton = std::make_unique<AboutLink>(editor, "", "Copy Version Info");
    copyButton->onClick = [this]() { copyInfo(); };
    copyButton->displayColor = theme::ColorMap::accent_2a;
    copyButton->hoverColor = theme::ColorMap::generic_content_high;
    copyButton->font = aboutFont;
    addAndMakeVisible(*copyButton);

    resetInfo();
}

AboutScreen::~AboutScreen() {}

void AboutScreen::resetInfo()
{
    info.clear();
    std::string ver = sst::plugininfra::VersionInformation::git_implied_display_version;
    ver += " / ";
    ver += sst::plugininfra::VersionInformation::project_version_and_hash;
    ver += " (JUCE " + std::to_string(JUCE_MAJOR_VERSION) + "." +
           std::to_string(JUCE_MINOR_VERSION) + "." + std::to_string(JUCE_BUILDNUMBER) + ")";
    info.push_back({"Version", ver, false});

    std::string platform = juce::SystemStats::getOperatingSystemName().toStdString();

    platform += " on " + sst::plugininfra::cpufeatures::brand();
    info.push_back({"System", platform, false});
    info.push_back({"Build",
                    std::string() + sst::plugininfra::VersionInformation::build_date + " " +
                        sst::plugininfra::VersionInformation::build_time + " with " +
                        sst::plugininfra::VersionInformation::cmake_compiler,
                    false});
    info.push_back({"Env",
                    fmt::format("{} at {} Hz", editor->engineStatus.runningEnvironment,
                                (int)editor->engineStatus.sampleRate),
                    false});

    if (openSourceText.empty())
    {
        openSourceText = connectors::resources::loadTextFile("opensource-credits.txt");
        openSourceInfo.clear();
        std::pair<std::string, std::string> tmp;
        std::stringstream stream(openSourceText);
        std::string line;
        bool onFirst{true};
        while (std::getline(stream, line))
        {
            if (line.empty())
            {
            }
            else if (onFirst)
            {
                tmp.first = line;
                onFirst = false;
            }
            else
            {
                tmp.second = line;
                onFirst = true;
                openSourceInfo.push_back(tmp);
            }
        }

        for (auto &inf : openSourceInfo)
        {
            auto as = std::make_unique<AboutLink>(editor, inf.first, inf.second);
            addAndMakeVisible(*as);
            openSourceLinkWidgets.push_back(std::move(as));
        }
    }
}
void AboutScreen::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.9f));
    float iconScale = 4.0;
    float iconHeight = 80; // iconHeight returns wrong thing?
    float iconWidth = 78;  // iconHeight returns wrong thing?
    float iconTransX = 00;
    float iconTransY = -10;
    auto cp = getLocalBounds().getCentre();
    cp.y -= 50;

    {
        auto gs = juce::Graphics::ScopedSaveState(g);
        auto c = (getHeight() - iconHeight * iconScale) / 2.0;

        g.addTransform(juce::AffineTransform().scaled(iconScale).translated(
            cp.x - iconWidth * iconScale / 2, cp.y - iconHeight * iconScale / 2));

        icon->drawAt(g, 0, 0, 1.0);
    }

    g.setColour(editor->themeColor(theme::ColorMap::generic_content_highest));
    g.setFont(titleFont);
    auto tp = (int)(cp.y + iconHeight * iconScale / 2) - 40;
    g.drawText("ShortcircuitXT", 0, tp, getWidth(), 50, juce::Justification::centred);
    tp += 55;

    g.setColour(editor->themeColor(theme::ColorMap::generic_content_high));
    g.setFont(subtitleFont);
    g.drawText("A personal creative sampler", 0, tp, getWidth(), 20, juce::Justification::centred);
    tp += 23;
    g.drawText("From the Surge Synth Team", 0, tp, getWidth(), 20, juce::Justification::centred);

    g.setFont(aboutFont);
    auto r = getLocalBounds().withHeight(20).translated(0, 320);
    r = r.withY(getHeight() - (info.size()) * infoh - 5);
    for (const auto &q : info)
    {
        g.setColour(editor->themeColor(theme::ColorMap::accent_1b));
        auto ra = r.withWidth(100).withTrimmedLeft(5);
        g.drawText(q.title, ra, juce::Justification::left);
        g.setColour(editor->themeColor(theme::ColorMap::generic_content_medium));
        auto rb = r.withTrimmedLeft(102);
        g.drawText(q.value, rb, juce::Justification::left);
        r = r.translated(0, infoh);
    }

    g.setColour(editor->themeColor(theme::ColorMap::generic_content_medium));
    g.drawText("ShortcircuitXT uses the following open source libraries:", openSourceX,
               openSourceStartY - 25, getWidth(), 25, juce::Justification::centredLeft);

    for (int i = 0; i < numButtons; i++)
    {
        {
            juce::Graphics::ScopedSaveState gs(g);

            auto r = buttonRects[i];
            bool isHovered = buttonRects[i].toFloat().contains(mpos);
            auto t = juce::AffineTransform();
            auto imoff = i > 3 ? 1 : 0; // clap wrapper doesn't do lv2
            t = t.translated(-iconSize * (i + imoff), -iconSize * isHovered)
                    .scaled(1.f * buttonSize / iconSize)
                    .translated(r.getX(), r.getY());
            g.reduceClipRegion(r);

            aboutLinkIcons->draw(g, 1.f, t);
        }
    }
}

void AboutScreen::copyInfo()
{
    std::ostringstream oss;
    oss << "Shortcircuit XT\n";
    for (const auto &i : info)
    {
        oss << i.title << ":\t" << i.value << "\n";
    }
    juce::SystemClipboard::copyTextToClipboard(oss.str());
}
void AboutScreen::resized()
{
    auto r = getLocalBounds()
                 .withHeight(infoh)
                 .withWidth(100)
                 .withY(getHeight() - (info.size() + 1.5) * infoh - 5)
                 .withTrimmedLeft(5);
    copyButton->setBounds(r.withWidth(400));

    auto wlh{15};
    auto wbox = getLocalBounds()
                    .withWidth(getWidth() * 0.4)
                    .withHeight(15)
                    .translated(openSourceX, openSourceStartY);
    for (const auto &w : openSourceLinkWidgets)
    {
        w->setBounds(wbox);
        wbox = wbox.translated(0, wlh);
    }

    int margin = 5;
    auto ba = getLocalBounds().reduced(margin).withHeight(buttonSize).withWidth(buttonSize);

    int x = 9; // number of iconized links

    buttonRects[0] = ba.translated(getWidth() - (margin + buttonSize) * numButtons, 0);

    for (int i = 1; i < numButtons; i++)
    {
        buttonRects[i] = buttonRects[i - 1].translated(margin + buttonSize, 0);
    }
}
void AboutScreen::visibilityChanged()
{
    if (isVisible())
        resetInfo();
}
} // namespace scxt::ui::app::other_screens