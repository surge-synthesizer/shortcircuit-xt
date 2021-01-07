/*
** ShortCircuit3 is Free and Open Source Software
**
** ShortCircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2021 by various individuals as described by the Git transaction log
**
** All source at: https://github.com/surge-synthesizer/surge.git
**
** ShortCircuit was a commercial product from 2004-2018, with Copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made ShortCircuit
** open source in December 2020.
*/

#include <sstream>

#include "ZoneKeyboardDisplay.h"

void ZoneKeyboardDisplay::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(200, 200, 240));
    g.setColour(juce::Colour(0, 0, 0));
    g.drawText("Zones", 2, 2, 200, 15, Justification::centredLeft);

    // This obviously sucks
    auto yp = 17;
    for (int i = 0; i < max_zones; ++i)
    {
        if (zsp->activezones[i])
        {
            std::ostringstream oss;
            oss << "z=" << i << " " << zsp->zonecopies[i].key_low << " "
                << zsp->zonecopies[i].key_high << std::endl;
            g.drawText(oss.str(), 2, yp, 200, 15, Justification::centredLeft);
            yp += 17;
        }
    }
}