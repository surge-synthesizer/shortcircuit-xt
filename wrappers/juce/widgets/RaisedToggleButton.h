//
// Created by Paul Walker on 1/5/22.
//

#ifndef SHORTCIRCUIT_RAISEDTOGGLEBUTTON_H
#define SHORTCIRCUIT_RAISEDTOGGLEBUTTON_H

#include "SCXTLookAndFeel.h"

struct RaisedToggleButton : public juce::Button
{
    RaisedToggleButton(const std::string &l) : label(l), juce::Button(l) {}
    virtual void paintButton(juce::Graphics &g, bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown)
    {
        auto c = juce::Colours::darkblue;
        if (shouldDrawButtonAsDown || getToggleState())
            c = juce::Colours::darkred;
        if (shouldDrawButtonAsHighlighted)
        {
            c = c.brighter(0.3);
        }
        g.setColour(c);
        g.fillRect(getLocalBounds());

        g.setColour(juce::Colours::white);
        g.drawRect(getLocalBounds());
        g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
        g.drawText(label, getLocalBounds(), juce::Justification::centred);
    }
    std::string label;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RaisedToggleButton);
};

#endif // SHORTCIRCUIT_RAISEDTOGGLEBUTTON_H
