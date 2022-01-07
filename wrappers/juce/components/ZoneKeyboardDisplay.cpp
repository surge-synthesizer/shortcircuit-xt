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

#include <sstream>

#include "ZoneKeyboardDisplay.h"
#include "interaction_parameters.h" // todo as where should these end up?

namespace SC3
{
namespace Components
{
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
    g.drawText("Drop Samples Here or Double Click to Load", getBounds(),
               juce::Justification::centred);
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
            if (editor->playingMidiNotes[i])
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
            if (editor->playingMidiNotes[i])
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
        if (editor->activeZones[i])
        {
            auto ks = editor->zonesCopy[i].key_low;
            auto ke = editor->zonesCopy[i].key_high;
            auto col = juce::Colour(zoneColorPallette[i % zoneColorPallette.size()]);
            if (i == hoveredZone)
            {
                col = juce::Colour(245, 184, 184);
            }
            auto xs = keyXBounds[ks].first;
            auto xe = keyXBounds[ke].second;

            g.setColour(col);
            g.fillRect(xs, zoneYStart, xe - xs, zoneYEnd - zoneYStart);
            g.setColour(zoneOutlineColor);
            g.drawRect(xs, zoneYStart, xe - xs, zoneYEnd - zoneYStart);
        }
    }
}
void ZoneKeyboardDisplay::mouseExit(const juce::MouseEvent &event)
{
    hoveredKey = -1;
    repaint();
}
void ZoneKeyboardDisplay::mouseMove(const juce::MouseEvent &event)
{
    int ohk = hoveredKey;
    int ohz = hoveredZone;
    int i = 0;
    hoveredKey = -1;
    hoveredZone = -1;
    for (auto r : keyLocations)
    {
        if (r.contains(event.x, event.y))
        {
            hoveredKey = i;
        }
        i++;
    }

    if (hoveredKey < 0)
    {
        // todo shared func should be getting zone locations for drawing as well as hit detection
        //  calculation of keyboard and zone locations should only be done on resize (not paint and
        //  not here)
        float keyboardHeight = 32; // todo to const
        float keyWidth = 1.f * getWidth() / 128;
        auto zoneYStart = keyboardHeight + 1;
        auto zoneYEnd = 1.f * getHeight();
        std::vector<std::pair<float, float>> keyXBounds;
        for (int i = 0; i < 128; ++i)
        {
            auto idx = i;
            float xpos = idx * keyWidth;
            keyXBounds.push_back(std::make_pair(xpos, xpos + keyWidth - 1));
        }
        for (int i = 0; i < max_zones; ++i)
        {
            if (editor->activeZones[i])
            {
                auto ks = editor->zonesCopy[i].key_low;
                auto ke = editor->zonesCopy[i].key_high;
                auto xs = keyXBounds[ks].first;
                auto xe = keyXBounds[ke].second;
                auto zoneRect =
                    juce::Rectangle<float>(xs, zoneYStart, xe - xs, zoneYEnd - zoneYStart);
                if (zoneRect.contains(event.x, event.y))
                {
                    hoveredZone = i;
                }
            }
        }
    }

    if (hoveredKey != ohk || hoveredZone != ohz)
    {
        repaint();
    }
}
void ZoneKeyboardDisplay::mouseDown(const juce::MouseEvent &event)
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
    else if (hoveredZone >= 0)
    {
        actiondata ad;
        ad.actiontype = vga_select_zone_primary;
        ad.id = ip_kgv_or_list;
        ad.data.i[0] = hoveredZone; // todo need zone id. is this it?
        sender->sendActionToEngine(ad);
    }
}
void ZoneKeyboardDisplay::mouseUp(const juce::MouseEvent &event)
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
void ZoneKeyboardDisplay::mouseDoubleClick(const juce::MouseEvent &event)
{
    if (hoveredKey >= 0)
        return;

    juce::FileChooser sampleChooser("Please choose a sample file",
                                    juce::File::getSpecialLocation(juce::File::userHomeDirectory));
    if (sampleChooser.browseForFileToOpen())
    {
        auto d = new DropList(); // engine takes ownership

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

} // namespace Components
} // namespace SC3
