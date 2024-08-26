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

#include "AboutScreen.h"
#include "utils.h"
#include "sst/plugininfra/cpufeatures.h"
#include "app/SCXTEditor.h"
#include "connectors/SCXTResources.h"

#include <version.h>

namespace scxt::ui::app::other_screens
{
struct AboutLink : juce::Component, HasEditor
{
    std::string url, txt;
    AboutLink(SCXTEditor *e, const std::string &url, const std::string tx)
        : HasEditor(e), url(url), txt(tx)
    {
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
        juce::URL(url).launchInDefaultBrowser();
    }
    void paint(juce::Graphics &g) override
    {
        if (isHovered)
        {
            g.setColour(editor->themeColor(theme::ColorMap::accent_2a));
        }
        else
        {
            g.setColour(editor->themeColor(theme::ColorMap::generic_content_medium));
        }
        g.setFont(editor->themeApplier.interLightFor(11));
        g.drawText(txt, getLocalBounds(), juce::Justification::centredLeft);
    }
};

AboutScreen::AboutScreen(SCXTEditor *e) : HasEditor(e)
{
    icon = connectors::resources::loadImageDrawable("images/SCicon.svg");
    auto interMed = connectors::resources::loadTypeface("fonts/Inter/static/Inter-Medium.ttf");
    titleFont = editor->themeApplier.interMediumFor(70);
    subtitleFont = editor->themeApplier.interMediumFor(30);
    infoFont = editor->themeApplier.interMediumFor(12);
    aboutFont = editor->themeApplier.interMediumFor(18);

    resetInfo();
}

AboutScreen::~AboutScreen() {}

void AboutScreen::resetInfo()
{
    info.clear();
    info.push_back({"Version", scxt::build::FullVersionStr, false});

    std::string platform = juce::SystemStats::getOperatingSystemName().toStdString();

    copyButton = std::make_unique<sst::jucegui::components::TextPushButton>();
    copyButton->setLabel("Copy Version Info");
    copyButton->setOnCallback([this]() { copyInfo(); });
    addAndMakeVisible(*copyButton);

    platform += " on " + sst::plugininfra::cpufeatures::brand();
    info.push_back({"System", platform, false});
    info.push_back({"Build",
                    std::string() + scxt::build::BuildDate + " " + scxt::build::BuildTime +
                        " with " + scxt::build::BuildCompiler,
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
    g.fillAll(editor->themeColor(theme::ColorMap::bg_1));
    float iconScale = 3.0;
    float iconHeight = 80; // iconHeight returns wrong thing?
    float iconWidth = 78;  // iconHeight returns wrong thing?
    float iconTransX = 00;
    float iconTransY = -10;
    {
        auto gs = juce::Graphics::ScopedSaveState(g);
        auto c = (getHeight() - iconHeight * iconScale) / 2.0;

        g.addTransform(
            juce::AffineTransform().scaled(iconScale).translated(iconTransX, iconTransY));
        icon->drawAt(g, 0, 0, 1.0);
    }

    g.setColour(editor->themeColor(theme::ColorMap::generic_content_highest));
    g.setFont(titleFont);
    g.drawText("ShortcircuitXT", 220, iconTransY + 25, getWidth(), getHeight(),
               juce::Justification::topLeft);
    g.setColour(editor->themeColor(theme::ColorMap::generic_content_high));
    g.setFont(subtitleFont);
    g.drawText("A personal creative sampler", 220, iconTransY + 90, getWidth(), getHeight(),
               juce::Justification::topLeft);
    g.drawText("From the Surge Synth Team", 220, iconTransY + 125, getWidth(), getHeight(),
               juce::Justification::topLeft);

    g.drawText("Logos and Stuff go here", 0, 5, getWidth() - 5, getHeight(),
               juce::Justification::topRight);

    g.setFont(aboutFont);
    auto r = getLocalBounds().withHeight(20).translated(0, 320);
    r = r.withY(getHeight() - (info.size()) * infoh - 5);
    for (const auto &q : info)
    {
        g.setColour(editor->themeColor(theme::ColorMap::accent_1a));
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
    copyButton->setBounds(r);

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
}
void AboutScreen::visibilityChanged()
{
    if (isVisible())
        resetInfo();
}
} // namespace scxt::ui::app::other_screens