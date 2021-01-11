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

    auto keyPressedColour = juce::Colour(0xFF, 0xc0, 0xcb);

    auto keyWhiteHoverColour = juce::Colour(200, 200, 255);
    auto keyBlackHoverColour = juce::Colour(200, 200, 255);
    std::vector<uint32_t> zoneColorPallette = {0xffdacac0, 0xffc7b582, 0xfff76f5e,
                                               0xff93965f, 0xffb1453f, 0xff676949};
    auto zoneOutlineColor = juce::Colour(240, 220, 200);
    // end configuration

    g.fillAll(juce::Colour(80, 70, 60));
    g.setColour(juce::Colour(255, 255, 255));
    g.drawText("Drop Samples Here or Double Click to Load", getBounds(), Justification::centred);
    g.setColour(juce::Colour(0, 0, 0));

    /*
     * White keys first
     */

    std::vector<std::pair<float, float>> keyXBounds;
    keyLocations.clear();
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
        if (isWhiteKey)
        {
            if (i == hoveredKey)
            {
                g.setColour(keyWhiteHoverColour);
                g.fillRect(xpos, ypos, keyWidth, ypos + keyboardHeight);
            }
            if (zsp->playingMidiNotes[i])
            {
                g.setColour(keyPressedColour);
                g.fillRect(xpos, ypos, keyWidth, ypos + keyboardHeight);
            }
            g.setColour(keyOutlineColour);
            if (on == 0 || on == 5)
                g.drawLine(xpos, ypos, xpos, keyboardHeight);
            keyLocations.push_back(juce::Rectangle(xpos, ypos, keyWidth, keyboardHeight));
        }
        else
        {
            if (zsp->playingMidiNotes[i])
            {
                g.setColour(keyPressedColour);
            }
            else if (i == hoveredKey)
            {
                g.setColour(keyBlackHoverColour);
            }
            else
            {
                g.setColour(keyBlackKeyColour);
            }
            g.fillRect(xpos, ypos, xpos + keyWidth, ypos + keyboardHeight - blackKeyInset);
            keyLocations.push_back(
                juce::Rectangle(xpos, ypos, xpos + keyWidth, keyboardHeight - blackKeyInset));
            g.setColour(keyOutlineColour);
            g.drawLine(xpos + keyWidth / 2, ypos + keyboardHeight - blackKeyInset,
                       xpos + keyWidth / 2, ypos + keyboardHeight, 1);
            g.drawLine(xpos, ypos, xpos, ypos + keyboardHeight - blackKeyInset);
            g.drawLine(xpos + keyWidth, ypos, xpos + keyWidth,
                       ypos + keyboardHeight - blackKeyInset);
            g.drawLine(xpos, ypos + keyboardHeight - blackKeyInset, xpos + keyWidth,
                       ypos + keyboardHeight - blackKeyInset);
        }
        g.setColour(keyOutlineColour);
        g.drawLine(xpos, keyboardHeight, xpos + keyWidth, keyboardHeight);
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
void ZoneKeyboardDisplay::mouseExit(const MouseEvent &event)
{
    hoveredKey = -1;
    repaint();
}
void ZoneKeyboardDisplay::mouseMove(const MouseEvent &event)
{
    int ohk = hoveredKey;
    int i = 0;
    hoveredKey = -1;
    for (auto r : keyLocations)
    {
        if (r.contains(event.x, event.y))
        {
            hoveredKey = i;
        }
        i++;
    }
    if (hoveredKey != ohk)
    {
        repaint();
    }
}
void ZoneKeyboardDisplay::mouseDown(const MouseEvent &event)
{
    if (hoveredKey >= 0)
    {
        playingKey = hoveredKey;
        actiondata ad;
        ad.actiontype = vga_note;
        ad.data.i[0] = playingKey;
        ad.data.i[1] = 127;
        // SEND
        sender->sendActionToEngine(ad);
    }
}
void ZoneKeyboardDisplay::mouseUp(const MouseEvent &event)
{
    if (playingKey >= 0)
    {
        playingKey = hoveredKey;
        actiondata ad;
        ad.actiontype = vga_note;
        ad.data.i[0] = playingKey;
        ad.data.i[1] = 0;
        sender->sendActionToEngine(ad);

        playingKey = -1;
    }
}
void ZoneKeyboardDisplay::mouseDoubleClick(const MouseEvent &event)
{
    juce::FileChooser sampleChooser("Please choose a sample file",
                                    juce::File::getSpecialLocation(juce::File::userHomeDirectory));
    if (sampleChooser.browseForFileToOpen())
    {
        auto d = new DropList();

        auto f = sampleChooser.getResult();
        auto fd = DropList::File();
        fd.p = string_to_path(f.getFullPathName().toStdString().c_str());
        d->files.push_back(fd);

        actiondata ad;
        ad.actiontype = vga_load_dropfiles;
        ad.data.dropList = d;
        sender->sendActionToEngine(ad);
    }
}
