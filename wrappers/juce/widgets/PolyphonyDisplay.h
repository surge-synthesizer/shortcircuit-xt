//
// Created by paul on 2/6/2022.
//

#ifndef SHORTCIRCUIT_POLYPHONYDISPLAY_H
#define SHORTCIRCUIT_POLYPHONYDISPLAY_H

#include "data/SCXTData.h"
#include "SCXTEditor.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace scxt
{
namespace widgets
{
struct PolyphonyDisplay : public juce::Component, scxt::data::UIStateProxy::Invalidatable
{
    PolyphonyDisplay(SCXTEditor *ed) : editor(ed){};

    void paint(juce::Graphics &g) override
    {
        if (poly == 0)
            g.setColour(juce::Colours::lightgrey);
        else
            g.setColour(juce::Colours::white);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
        g.drawText(std::to_string(poly), getLocalBounds(), juce::Justification::centred);
    }

    void onProxyUpdate() override
    {
        if (poly != editor->polyphony)
        {
            poly = editor->polyphony;
            repaint();
        }
    }

    int poly{0};
    SCXTEditor *editor{nullptr};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolyphonyDisplay);
};
} // namespace widgets
} // namespace scxt

#endif // SHORTCIRCUIT_POLYPHONYDISPLAY_H
