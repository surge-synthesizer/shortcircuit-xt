//
// Created by Paul Walker on 1/6/22.
//

#ifndef SHORTCIRCUIT_ABOUTPAGE_H
#define SHORTCIRCUIT_ABOUTPAGE_H

#include "PageBase.h"
#include "version.h"

namespace SC3
{

namespace Pages
{
struct AboutPage : PageBase
{
    AboutPage(SC3Editor *ed, SC3Editor::Pages p) : PageBase(ed, p) {}

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::darkred);
        auto r = getLocalBounds().withHeight(60).translated(0, 200);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(40));
        g.setColour(juce::Colours::white);
        g.drawText("ShortCircuit XT", r, juce::Justification::centred);
        r = r.translated(0, 60);
        g.drawText(SC3::Build::FullVersionStr, r, juce::Justification::centred);
        r = r.translated(0, 60);
        std::string dt = SC3::Build::BuildDate;
        dt += " at ";
        dt += SC3::Build::BuildTime;
        g.drawText(dt, r, juce::Justification::centred);
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutPage);
};
} // namespace Pages
} // namespace SC3

#endif // SHORTCIRCUIT_ABOUTPAGE_H
