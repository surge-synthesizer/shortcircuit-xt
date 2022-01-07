//
// Created by Paul Walker on 1/6/22.
//

#ifndef SHORTCIRCUIT_ABOUTPAGE_H
#define SHORTCIRCUIT_ABOUTPAGE_H

#include "PageBase.h"
#include "version.h"
#include "BinaryUIAssets.h"

namespace SC3
{

namespace Pages
{
struct AboutPage : PageBase
{
    AboutPage(SC3Editor *ed, SC3Editor::Pages p) : PageBase(ed, p)
    {
        icon = juce::Drawable::createFromImageData(SCXTUIAssets::SCicon_svg,
                                                   SCXTUIAssets::SCicon_svgSize);
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::darkred);
        g.setColour(juce::Colours::white);
        {
            auto gs = juce::Graphics::ScopedSaveState(g);
            auto wh = std::max(icon->getWidth(), icon->getHeight());
            auto c = (getWidth() - icon->getWidth() * 3) / 2.0;
            g.addTransform(juce::AffineTransform().scaled(3.0).translated(c, 20));

            icon->drawAt(g, 0, 0, 1.0);
        }
        auto r = getLocalBounds().withHeight(60).translated(0, 220);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(60));
        g.setColour(juce::Colours::white);
        g.drawText("ShortCircuit XT", r, juce::Justification::centred);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(40));
        r = r.translated(0, 60);
        g.drawText(SC3::Build::FullVersionStr, r, juce::Justification::centred);
        r = r.translated(0, 60);
        std::string dt = SC3::Build::BuildDate;
        dt += " at ";
        dt += SC3::Build::BuildTime;
        g.drawText(dt, r, juce::Justification::centred);
    }

    std::unique_ptr<juce::Drawable> icon;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutPage);
};
} // namespace Pages
} // namespace SC3

#endif // SHORTCIRCUIT_ABOUTPAGE_H
