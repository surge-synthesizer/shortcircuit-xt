//
// Created by Paul Walker on 1/5/22.
//

#ifndef SHORTCIRCUIT_SCXTLOOKANDFEEL_H
#define SHORTCIRCUIT_SCXTLOOKANDFEEL_H

#include "juce_gui_basics/juce_gui_basics.h"
#include <unordered_map>

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
    vuClip,

    fxPanelHeaderBackground,
    fxPanelHeaderText,
    fxPanelBackground
    // clang-format on
};

template <typename T> struct ColorRemapper
{
    std::unordered_map<uint32_t, uint32_t> remaps;
    void remapColour(uint32_t underlyer, uint32_t replace) { remaps[underlyer] = replace; }
    inline T *asT() { return static_cast<T *>(this); }

    juce::Colour findRemappedColour(uint32_t id)
    {
        if (remaps.find(id) != remaps.end())
            id = remaps[id];
        return asT()->findColour(id);
    }
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

    void drawComboBox(juce::Graphics &graphics, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox &box) override;
    juce::Font getComboBoxFont(juce::ComboBox &) override { return getMonoFontAt(10); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SCXTLookAndFeel);
};

#endif // SHORTCIRCUIT_SCXTLOOKANDFEEL_H
