/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_ABOUTPAGE_H
#define SHORTCIRCUIT_ABOUTPAGE_H

#include "PageBase.h"
#include "version.h"
#include "BinaryUIAssets.h"
#include "sst/plugininfra/paths.h"
#include "sst/plugininfra/cpufeatures.h"

namespace scxt
{

namespace pages
{
struct AboutPage : PageBase
{
    struct AboutInfo
    {
        std::string title;
        std::string value;
        bool isBig{true};
    };
    std::vector<AboutInfo> info;
    AboutPage(SCXTEditor *ed, SCXTEditor::Pages p) : PageBase(ed, p, scxt::style::Selector{"about"})
    {
        icon = juce::Drawable::createFromImageData(SCXTUIAssets::SCicon_svg,
                                                   SCXTUIAssets::SCicon_svgSize);
        info.push_back({"Version", scxt::build::FullVersionStr});
        info.push_back({"Build Time", std::string(scxt::build::BuildDate) + " at " +
                                          scxt::build::BuildTime + " using " +
                                          scxt::build::BuildCompiler});

#if MAC
        std::string platform = "macOS";
#elif WINDOWS
        std::string platform = "Windows";
#elif LINUX
        std::string platform = "Linux";
#else
        std::string platform = "GLaDOS, Orac or Skynet";
#endif
        std::string bitness =
            (sizeof(size_t) == 4 ? std::string("32") : std::string("64")) + "-bit";
        std::string wrapper = ed->audioProcessor.sc3->wrapperType;

        info.push_back({"System", platform + " " + bitness + " " + wrapper + " on " +
                                      sst::plugininfra::cpufeatures::brand()});
        info.push_back(
            {"Executable", sst::plugininfra::paths::sharedLibraryBinaryPath().u8string()});
        info.push_back({"User Dir", ed->audioProcessor.sc3->userDocumentDirectory.u8string()});
        copyButton = std::make_unique<juce::TextButton>("Copy");
        copyButton->setButtonText("Copy");
        copyButton->onClick = [this]() { copyInfo(); };
        addAndMakeVisible(*copyButton);
    }

    static constexpr int infoh = 15;
    void resized() override
    {
        auto r = getLocalBounds()
                     .withHeight(infoh)
                     .withWidth(100)
                     .withY(getHeight() - (info.size() + 1.5) * infoh - 5)
                     .withTrimmedLeft(5);
        copyButton->setBounds(r);
    }

    void copyInfo()
    {
        std::ostringstream oss;
        oss << "Shortcircuit XT\n";
        for (auto i : info)
        {
            oss << i.title << ":\t" << i.value << "\n";
        }
        juce::SystemClipboard::copyTextToClipboard(oss.str());
    }
    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colour(0xFF151515));
        g.setColour(juce::Colours::white);
        {
            auto gs = juce::Graphics::ScopedSaveState(g);
            auto wh = std::max(icon->getWidth(), icon->getHeight());
            auto c = (getWidth() - icon->getWidth() * 3) / 2.0;
            g.addTransform(juce::AffineTransform().scaled(3.0).translated(c, 120));

            icon->drawAt(g, 0, 0, 1.0);
        }
        auto r = getLocalBounds().withHeight(60).translated(0, 320);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(60));
        g.setColour(juce::Colours::white);
        g.drawText("ShortCircuit XT", r, juce::Justification::centred);

        g.setFont(SCXTLookAndFeel::getMonoFontAt(12));

        r = r.withHeight(infoh);
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

    std::unique_ptr<juce::Drawable> icon;
    std::unique_ptr<juce::TextButton> copyButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutPage);
};
} // namespace pages
} // namespace scxt

#endif // SHORTCIRCUIT_ABOUTPAGE_H
