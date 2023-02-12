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

#ifndef SHORTCIRCUIT_POLYPHONYDISPLAY_H
#define SHORTCIRCUIT_POLYPHONYDISPLAY_H

#include "data/SCXTData.h"
#include "SCXTPluginEditor.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace scxt
{
namespace widgets
{
struct PolyphonyDisplay : public juce::Component, scxt::data::UIStateProxy::Invalidatable
{
    PolyphonyDisplay(SCXTEditor *ed) : editor(ed){};

    void paint(juce::Graphics &g) override
    {
        if (poly == 0)
            g.setColour(juce::Colours::lightgrey);
        else
            g.setColour(juce::Colours::white);
        g.setFont(SCXTLookAndFeel::getMonoFontAt(9));
        g.drawText(std::to_string(poly), getLocalBounds(), juce::Justification::centred);
    }

    void onProxyUpdate() override
    {
        if (poly != editor->polyphony)
        {
            poly = editor->polyphony;
            repaint();
        }
    }

    int poly{0};
    SCXTEditor *editor{nullptr};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolyphonyDisplay);
};
} // namespace widgets
} // namespace scxt

#endif // SHORTCIRCUIT_POLYPHONYDISPLAY_H
