//
// Created by Paul Walker on 1/22/22.
//

#ifndef SHORTCIRCUIT_SAMPLESIDEBAR_H
#define SHORTCIRCUIT_SAMPLESIDEBAR_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "SCXTLookAndFeel.h"

namespace scxt
{
namespace components
{
class BrowserSidebar : public juce::Component
{
    void paint(juce::Graphics &g) override
    {
        g.fillAll(juce::Colour(0xFF000020));
        auto hb = getLocalBounds().withHeight(20);
        SCXTLookAndFeel::fillWithGradientHeaderBand(g, hb, juce::Colour(0xFF774444));
        g.setFont(SCXTLookAndFeel::getMonoFontAt(10));
        g.setColour(juce::Colours::white);
        g.drawText("Samples and Patches", hb, juce::Justification::centred);
    }
};
} // namespace components
} // namespace scxt

#endif // SHORTCIRCUIT_SAMPLESIDEBAR_H
