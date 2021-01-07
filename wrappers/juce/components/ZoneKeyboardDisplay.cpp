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
    float keyboardHeight = 32;
    float keyWidth = 1.f * getWidth() / 128;
    float blackKeyInset = keyboardHeight / 4;
    auto keyOutlineColour = juce::Colour(20, 20, 20);
    auto keyWhiteKeyColour = juce::Colour(255, 255, 245);
    auto keyBlackKeyColour = juce::Colour(40, 40, 40);
    std::vector<uint32_t> zoneColorPallette = {0xffdacac0, 0xffc7b582, 0xfff76f5e,
                                               0xff93965f, 0xffb1453f, 0xff676949};
    auto zoneOutlineColor = juce::Colour(240, 220, 200);
    // end configuration

    g.fillAll(juce::Colour(80, 70, 60));
    g.setColour(juce::Colour(0, 0, 0));

    /*
     * White keys first
     */

    std::vector<std::pair<float, float>> keyXBounds;
    for (int i = 0; i < 128; ++i)
    {
        bool isWhiteKey = false;
        int on = i % 12;
        if (on == 0 || on == 2 || on == 4 || on == 5 || on == 7 || on == 9 || on == 11)
            isWhiteKey = true;

        auto idx = i;
        float xpos = idx * keyWidth;

        keyXBounds.push_back(std::make_pair(xpos, xpos + keyWidth - 1));

        float ypos = 0.0;
        g.setColour(keyWhiteKeyColour);
        g.fillRect(xpos, ypos, keyWidth, ypos + keyboardHeight);
        g.setColour(keyOutlineColour);
        g.drawLine(xpos, keyboardHeight, xpos + keyWidth, keyboardHeight);
        if (isWhiteKey)
        {
            g.setColour(keyOutlineColour);
            if (on == 0 || on == 5)
                g.drawLine(xpos, ypos, xpos, keyboardHeight);
        }
        else
        {
            g.setColour(keyBlackKeyColour);
            g.fillRect(xpos, ypos, xpos + keyWidth, ypos + keyboardHeight - blackKeyInset);
            g.setColour(keyOutlineColour);
            g.drawLine(xpos + keyWidth / 2, ypos + keyboardHeight - blackKeyInset,
                       xpos + keyWidth / 2, ypos + keyboardHeight, 1);
            g.drawLine(xpos, ypos, xpos, ypos + keyboardHeight - blackKeyInset);
            g.drawLine(xpos + keyWidth, ypos, xpos + keyWidth,
                       ypos + keyboardHeight - blackKeyInset);
            g.drawLine(xpos, ypos + keyboardHeight - blackKeyInset, xpos + keyWidth,
                       ypos + keyboardHeight - blackKeyInset);
        }
    }

    auto zoneYStart = keyboardHeight + 1;
    auto zoneYEnd = 1.f * getHeight();

    // This obviously sucks

    for (int i = 0; i < max_zones; ++i)
    {
        if (zsp->activezones[i])
        {
            auto ks = zsp->zonecopies[i].key_low;
            auto ke = zsp->zonecopies[i].key_high;
            auto col = juce::Colour(zoneColorPallette[i % zoneColorPallette.size()]);
            auto xs = keyXBounds[ks].first;
            auto xe = keyXBounds[ke].second;

            g.setColour(col);
            g.fillRect(xs, zoneYStart, xe - xs, zoneYEnd - zoneYStart);
            g.setColour(zoneOutlineColor);
            g.drawRect(xs, zoneYStart, xe - xs, zoneYEnd - zoneYStart);
        }
    }
}