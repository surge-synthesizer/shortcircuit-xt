/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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
#include "SCXTBinary.h"
#include "utils.h"
#include "sst/plugininfra/cpufeatures.h"
#include "SCXTEditor.h"

#include <version.h>

namespace scxt::ui
{
AboutScreen::AboutScreen(SCXTEditor *e) : HasEditor(e)
{
    icon = juce::Drawable::createFromImageData(binary::SCicon_svg, binary::SCicon_svgSize);

    auto interMed = juce::Typeface::createSystemTypefaceFor(scxt::ui::binary::InterMedium_ttf,
                                                            scxt::ui::binary::InterMedium_ttfSize);

    titleFont = juce::Font(interMed);
    titleFont.setHeight(70);
    subtitleFont = juce::Font(interMed);
    subtitleFont.setHeight(30);
    infoFont = juce::Font(interMed);
    infoFont.setHeight(14);
    aboutFont = juce::Font(interMed);
    aboutFont.setHeight(20);

    resetInfo();
}

void AboutScreen::resetInfo()
{
    info.clear();
    info.push_back({"Version", scxt::build::FullVersionStr, false});

    std::string platform = juce::SystemStats::getOperatingSystemName().toStdString();

    copyButton = std::make_unique<juce::TextButton>("Copy");
    copyButton->setButtonText("Copy");
    copyButton->onClick = [this]() { copyInfo(); };
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
}
void AboutScreen::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::black);
    float iconScale = 4.0;
    float iconHeight = 80; // iconHeightr eturns wrong thing?
    float iconWidth = 78;  // iconHeightr eturns wrong thing?
    float iconTransX = 20;
    float iconTransY = 148;
    {
        auto gs = juce::Graphics::ScopedSaveState(g);
        auto c = (getHeight() - iconHeight * iconScale) / 2.0;

        g.addTransform(
            juce::AffineTransform().scaled(iconScale).translated(iconTransX, iconTransY));
        icon->drawAt(g, 0, 0, 1.0);
        g.setColour(juce::Colours::red);
    }

    g.setColour(juce::Colours::white);
    g.setFont(titleFont);
    g.drawText("ShortcircuitXT", 330, iconTransY + 45, getWidth(), getHeight(),
               juce::Justification::topLeft);
    g.setFont(subtitleFont);
    g.drawText("A personal creative sampler", 330, iconTransY + 110, getWidth(), getHeight(),
               juce::Justification::topLeft);
    g.drawText("From the Surge Synth Team", 330, iconTransY + 145, getWidth(), getHeight(),
               juce::Justification::topLeft);

    g.setFont(infoFont);
    g.drawText("Licenses and Credits go Here", 5, 5, getWidth(), getHeight(),
               juce::Justification::topLeft);

    g.drawText("Logos and Stuff go here", 0, 5, getWidth() - 5, getHeight(),
               juce::Justification::topRight);

    g.setFont(aboutFont);
    auto r = getLocalBounds().withHeight(20).translated(0, 320);
    r = r.withY(getHeight() - (info.size()) * infoh - 5);
    for (auto q : info)
    {
        g.setColour(juce::Colour(0xFFFF9000));
        auto ra = r.withWidth(100).withTrimmedLeft(5);
        g.drawText(q.title, ra, juce::Justification::left);
        g.setColour(juce::Colours::white);
        auto rb = r.withTrimmedLeft(102);
        g.drawText(q.value, rb, juce::Justification::left);
        r = r.translated(0, infoh);
    }
}

void AboutScreen::copyInfo()
{
    std::ostringstream oss;
    oss << "Shortcircuit XT\n";
    for (auto i : info)
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
}
void AboutScreen::visibilityChanged()
{
    if (isVisible())
        resetInfo();
}
} // namespace scxt::ui