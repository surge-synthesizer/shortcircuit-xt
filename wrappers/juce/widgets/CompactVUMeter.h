//
// Created by Paul Walker on 1/6/22.
//

#ifndef SHORTCIRCUIT_COMPACTVUMETER_H
#define SHORTCIRCUIT_COMPACTVUMETER_H

#include "DataInterfaces.h"
#include "SC3Editor.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace SC3
{
namespace Widgets
{
struct CompactVUMeter : public juce::Component, UIStateProxy::Invalidatable
{
    CompactVUMeter(SC3Editor *ed) : editor(ed){};

    void paint(juce::Graphics &g) override
    {
        g.fillAll(findColour(SCXTColours::vuBackground));
        auto top = getLocalBounds().withHeight(getHeight() / 2).reduced(1, 1);
        auto bot = getLocalBounds().withTrimmedTop(getHeight() / 2 + 1).reduced(1, 1);

        if (clipped[0])
            g.setColour(findColour(SCXTColours::vuClip));
        else
            g.setGradientFill(juce::ColourGradient::horizontal(
                findColour(SCXTColours::vuPlayLow), findColour(SCXTColours::vuPlayHigh), top));
        g.fillRect(top);
        top = top.withTrimmedLeft(getWidth() * value[0] / 255);
        g.setColour(findColour(SCXTColours::vuBackground));
        g.fillRect(top);

        if (clipped[1])
            g.setColour(findColour(SCXTColours::vuClip));
        else
            g.setGradientFill(juce::ColourGradient::horizontal(
                findColour(SCXTColours::vuPlayLow), findColour(SCXTColours::vuPlayHigh), bot));
        g.fillRect(bot);
        bot = bot.withTrimmedLeft(getWidth() * value[1] / 255);
        g.setColour(findColour(SCXTColours::vuBackground));
        g.fillRect(bot);

        g.setColour(findColour(SCXTColours::vuOutline));
        g.drawRect(getLocalBounds());
    }

    void onProxyUpdate() override
    {
        value[0] = editor->vuData[0].ch1;
        value[1] = editor->vuData[0].ch2;
        clipped[0] = editor->vuData[0].clip1;
        clipped[1] = editor->vuData[0].clip2;
    }

    bool clipped[2]{false, false};
    uint8_t value[2]{0, 0};
    SC3Editor *editor{nullptr};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactVUMeter);
};
} // namespace Widgets
} // namespace SC3

#endif // SHORTCIRCUIT_COMPACTVUMETER_H
