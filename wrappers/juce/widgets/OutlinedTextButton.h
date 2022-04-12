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

#ifndef SHORTCIRCUIT_OUTLINEDTEXTBUTTON_H
#define SHORTCIRCUIT_OUTLINEDTEXTBUTTON_H

#include "juce_gui_basics/juce_gui_basics.h"
#include "SCXTLookAndFeel.h"
#include "style/StyleSheet.h"

namespace scxt
{
namespace widgets
{
struct OutlinedTextButton : public juce::TextButton,
                            style::DOMParticipant,
                            ColorRemapper<OutlinedTextButton>
{
    enum ColourIds
    {
        upColour = 0x000a0010,
        downColour,
        textColour
    };
    OutlinedTextButton(const std::string &t) : juce::TextButton(t), DOMParticipant("outlined_text")
    {
        setIsDOMContainer(false);
    }

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
