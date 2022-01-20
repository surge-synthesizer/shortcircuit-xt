//
// Created by Paul Walker on 1/6/22.
//

#ifndef SHORTCIRCUIT_OUTLINEDTEXTBUTTON_H
#define SHORTCIRCUIT_OUTLINEDTEXTBUTTON_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "SCXTLookAndFeel.h"

namespace scxt
{
namespace widgets
{
struct OutlinedTextButton : public juce::TextButton, ColorRemapper<OutlinedTextButton>
{
    enum ColourIds
    {
        upColour = 0x000a0010,
        downColour,
        textColour
    };
    OutlinedTextButton(const std::string &t) : juce::TextButton(t) {}

    void paintButton(juce::Graphics &g, bool highlighted, bool down) override
    {
        auto c = findRemappedColour(upColour);
        if (down || getToggleState())
            c = findRemappedColour(downColour);
        if (highlighted)
        {
            c = c.brighter(0.3);
        }

        if (!isEnabled())
            c = juce::Colour(0xFF777777);

        SCXTLookAndFeel::fillWithRaisedOutline(g, getLocalBounds(), c, down || getToggleState());
        g.setColour(findRemappedColour(textColour));
        if (!isEnabled())
            g.setColour(juce::Colour(0xFFAAAAAA));
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        g.drawText(getButtonText(), getLocalBounds(), juce::Justification::centred);
    }
};
} // namespace widgets
} // namespace scxt
#endif // SHORTCIRCUIT_OUTLINEDTEXTBUTTON_H
