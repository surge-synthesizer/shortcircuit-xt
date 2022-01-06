//
// Created by Paul Walker on 1/5/22.
//

#ifndef SHORTCIRCUIT_SCXTLOOKANDFEEL_H
#define SHORTCIRCUIT_SCXTLOOKANDFEEL_H

#include "juce_gui_basics/juce_gui_basics.h"

struct SCXTLookAndFeel : public juce::LookAndFeel_V4
{
    SCXTLookAndFeel();
    ~SCXTLookAndFeel() = default;

    static juce::Font getMonoFontAt(int sz);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SCXTLookAndFeel);
};

#endif // SHORTCIRCUIT_SCXTLOOKANDFEEL_H
