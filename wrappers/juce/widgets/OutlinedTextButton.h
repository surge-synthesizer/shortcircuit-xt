//
// Created by Paul Walker on 1/6/22.
//

#ifndef SHORTCIRCUIT_OUTLINEDTEXTBUTTON_H
#define SHORTCIRCUIT_OUTLINEDTEXTBUTTON_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "SCXTLookAndFeel.h"

namespace SC3
{
namespace Widgets
{
struct OutlinedTextButton : public juce::TextButton
{
    OutlinedTextButton(const std::string &t) : juce::TextButton(t) {}

    void paintButton(juce::Graphics &g, bool highlighted, bool down) override
    {
        auto c = juce::Colours::darkblue;
        if (down || getToggleState())
            c = juce::Colours::darkred;
        if (highlighted)
        {
            c = c.brighter(0.3);
        }
        g.setColour(c);
        g.fillRect(getLocalBounds());

        g.setColour(juce::Colours::white);
        g.drawRect(getLocalBounds());
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        g.drawText(getButtonText(), getLocalBounds(), juce::Justification::centred);
    }
};
} // namespace Widgets
} // namespace SC3
#endif // SHORTCIRCUIT_OUTLINEDTEXTBUTTON_H
