/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef SHORTCIRCUIT_STUBREGION_H
#define SHORTCIRCUIT_STUBREGION_H

#include <JuceHeader.h>

class StubRegion : public juce::Component
{
  public:
    StubRegion(const juce::Rectangle<int> &rect, const juce::Colour &c, const juce::String &lab)
        : juce::Component()
    {
        setBounds(rect);
        this->bg = c;
        this->lab = lab;
    }

    void paint(Graphics &g) override
    {
        g.fillAll(bg);
        g.setColour(juce::Colour(255, 255, 255));
        g.drawText(lab, getBounds(), Justification::centred);
    }

    juce::Colour bg;
    juce::String lab;
};

#endif // SHORTCIRCUIT_STUBREGION_H
