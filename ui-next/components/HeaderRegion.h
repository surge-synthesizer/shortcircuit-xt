//
// Created by Paul Walker on 2/7/23.
//

#ifndef SCXT_UI_COMPONENTS_HEADERREGION_H
#define SCXT_UI_COMPONENTS_HEADERREGION_H

#include <juce_gui_basics/juce_gui_basics.h>

namespace scxt::ui
{
struct SCXTEditor;

struct HeaderRegion : juce::Component
{
    std::unique_ptr<juce::Button> multiPage, sendPage;

    SCXTEditor *editor{nullptr};
    HeaderRegion(SCXTEditor *);

    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colours::pink);
        g.setColour(juce::Colours::pink.contrasting());
        g.drawText("HEADER", getLocalBounds(), juce::Justification::centred);
    }

    void resized() override
    {
        multiPage->setBounds(2, 2, 98, getHeight() - 4);
        sendPage->setBounds(102, 2, 98, getHeight() - 4);
    }
};
} // namespace scxt::ui

#endif // SHORTCIRCUIT_HEADERREGION_H
