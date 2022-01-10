//
// Created by Paul Walker on 1/6/22.
//

#ifndef SHORTCIRCUIT_PAGEBASE_H
#define SHORTCIRCUIT_PAGEBASE_H

#include "SCXTEditor.h"
#include "SCXTLookAndFeel.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace scxt
{
namespace pages
{
struct PageBase : public juce::Component
{
    PageBase(SCXTEditor *ed, const SCXTEditor::Pages &p) : editor(ed), page(p) {}
    virtual ~PageBase() = default;

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::darkred);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(40));
        g.setColour(juce::Colours::white);
        g.drawText(SCXTEditor::pageName(page), getLocalBounds(), juce::Justification::centred);
    }

    virtual void connectProxies() {}

    SCXTEditor *editor{nullptr};
    SCXTEditor::Pages page{SCXTEditor::ZONE};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageBase);
};
} // namespace pages
} // namespace scxt
#endif // SHORTCIRCUIT_PAGEBASE_H
