//
// Created by Paul Walker on 1/5/22.
//

#ifndef SHORTCIRCUIT_SCXTLOOKANDFEEL_H
#define SHORTCIRCUIT_SCXTLOOKANDFEEL_H

#include "juce_gui_basics/juce_gui_basics.h"

enum SCXTColours : int32_t
{
    // clang-format off
    headerBackground = 0x0F002112,
    headerButton,
    headerButtonDown,
    headerButtonText,

    vuBackground,
    vuOutline,
    vuPlayLow,
    vuPlayHigh,
    vuClip
    // clang-format on
};

struct SCXTLookAndFeel : public juce::LookAndFeel_V4
{
    SCXTLookAndFeel();
    ~SCXTLookAndFeel() = default;

    static juce::Font getMonoFontAt(int sz);
    static void fillWithRaisedOutline(juce::Graphics &g, const juce::Rectangle<int> &r,
                                      juce::Colour base, bool down);
    static void fillWithGradientHeaderBand(juce::Graphics &g, const juce::Rectangle<int> &r,
                                           juce::Colour base);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SCXTLookAndFeel);
};

#endif // SHORTCIRCUIT_SCXTLOOKANDFEEL_H
