/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_COMPACTVUMETER_H
#define SHORTCIRCUIT_COMPACTVUMETER_H

#include "data/SCXTData.h"
#include "SCXTPluginEditor.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace scxt
{
namespace widgets
{
struct CompactVUMeter : public juce::Component, scxt::data::UIStateProxy::Invalidatable
{
    CompactVUMeter(SCXTEditor *ed) : editor(ed){};

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
    SCXTEditor *editor{nullptr};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactVUMeter);
};
} // namespace widgets
} // namespace scxt

#endif // SHORTCIRCUIT_COMPACTVUMETER_H
