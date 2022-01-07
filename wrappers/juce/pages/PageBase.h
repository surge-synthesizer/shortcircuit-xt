//
// Created by Paul Walker on 1/6/22.
//

#ifndef SHORTCIRCUIT_PAGEBASE_H
#define SHORTCIRCUIT_PAGEBASE_H

#include "SC3Editor.h"
#include "SCXTLookAndFeel.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace SC3
{
namespace Pages
{
struct PageBase : public juce::Component
{
    PageBase(SC3Editor *ed, const SC3Editor::Pages &p) : editor(ed), page(p) {}
    virtual ~PageBase() = default;

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::darkred);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(40));
        g.setColour(juce::Colours::white);
        g.drawText(SC3Editor::pageName(page), getLocalBounds(), juce::Justification::centred);
    }

    virtual void connectProxies() {}

    SC3Editor *editor{nullptr};
    SC3Editor::Pages page{SC3Editor::ZONE};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageBase);
};
} // namespace Pages
} // namespace SC3
#endif // SHORTCIRCUIT_PAGEBASE_H
