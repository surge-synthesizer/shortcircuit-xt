//
// Created by Paul Walker on 2/7/23.
//

#ifndef SCXT_UI_COMPONENTS_SENDFXSCREEN_H
#define SCXT_UI_COMPONENTS_SENDFXSCREEN_H

#include <juce_gui_basics/juce_gui_basics.h>

namespace scxt::ui
{
struct SendFXScreen : juce::Component
{
    void paint(juce::Graphics &g) {
        g.fillAll(juce::Colours::green);
        g.setColour(juce::Colours::green.contrasting());
        g.drawText("Send FX", getLocalBounds(), juce::Justification::centred);
    }
};
} // namespace scxt::ui
#endif // SHORTCIRCUIT_SENDFXSCREEN_H
