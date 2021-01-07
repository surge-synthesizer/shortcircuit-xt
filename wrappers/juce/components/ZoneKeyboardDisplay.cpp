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
    // Configuration which would later come from a L&F
    int totalWhiteKeys = round((128 / 12.0) * 7);
    float keyboardHeight = 32;
    float keyWidth = 1.f * getWidth() / totalWhiteKeys;
    float blackKeyInset = keyboardHeight / 4;
    float blackKeyWidth = keyWidth * 0.8;
    auto keyOutlineColour = juce::Colour(20, 20, 20);
    auto keyWhiteKeyColour = juce::Colour(255, 255, 245);
    auto keyBlackKeyColour = juce::Colour(40, 40, 40);
    // end configuration

    g.fillAll(juce::Colour(200, 200, 240));
    g.setColour(juce::Colour(0, 0, 0));

    /*
     * Draw the keyboard
     */
    int keyCenter = 60;
    auto pxCenter = getWidth() / 2.0;
    auto key60Start = pxCenter - keyWidth / 2.0;

    std::vector<int> whiteKeyIndex, blackKeyIndex;
    int wc = 0;
    for (int i = 0; i < 128; ++i)
    {
        int on = i % 12;
        if (on == 0 || on == 2 || on == 4 || on == 5 || on == 7 || on == 9 || on == 11)
        {
            whiteKeyIndex.push_back(wc++);
            blackKeyIndex.push_back(-1);
        }
        else
        {
            blackKeyIndex.push_back(wc);
            whiteKeyIndex.push_back(-1);
        }
    }

    /*
     * White keys first
     */
    for (int i = 0; i < 128; ++i)
    {
        auto idx = whiteKeyIndex[i];
        if (idx < 0)
            continue;
        float xpos = idx * keyWidth;
        float ypos = 0.0;
        g.setColour(keyWhiteKeyColour);
        g.fillRect(xpos, ypos, keyWidth, keyboardHeight);
        g.setColour(keyOutlineColour);
        g.drawRect(xpos, ypos, keyWidth, keyboardHeight);
    }
    /*
     * Then black keys
     */
    for (int i = 0; i < 128; ++i)
    {
        auto idx = blackKeyIndex[i];
        if (idx < 0)
            continue;
        auto xpos = idx * keyWidth - blackKeyWidth * 0.5;
        auto ypos = 0.0;
        g.setColour(keyBlackKeyColour);
        g.fillRect(xpos, ypos, blackKeyWidth, keyboardHeight - blackKeyInset);
        g.setColour(keyOutlineColour);
        g.drawRect(xpos, ypos, blackKeyWidth, keyboardHeight - blackKeyInset);
    }

    auto yp = keyboardHeight + 2;
    g.drawText("Zones", 2, yp, 200, 15, Justification::centredLeft);
    yp += 17;

    // This obviously sucks
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