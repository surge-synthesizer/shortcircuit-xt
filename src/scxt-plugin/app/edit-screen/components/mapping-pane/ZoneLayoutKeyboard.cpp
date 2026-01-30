/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "ZoneLayoutKeyboard.h"

#include "app/SCXTEditor.h"
#include "app/edit-screen/components/mapping-pane/MappingDisplay.h"
#include "messaging/messaging.h"

namespace scxt::ui::app::edit_screen
{
namespace cmsg = scxt::messaging::client;

ZoneLayoutKeyboard::ZoneLayoutKeyboard(MappingDisplay *d) : HasEditor(d->editor), display(d) {}

void ZoneLayoutKeyboard::paint(juce::Graphics &g)
{
    std::array<int, 128> midiState; // 0 == 0ff, 1 == gated, 2 == sounding
    std::fill(midiState.begin(), midiState.end(), 0);
    for (const auto &vd : display->editor->sharedUiMemoryState.voiceDisplayItems)
    {
        if (vd.active && vd.midiNote >= 0)
        {
            midiState[vd.midiNote] = vd.gated ? 1 : 2;
        }
    }
    for (int i = firstMidiNote; i < lastMidiNote; ++i)
    {
        auto n = i % 12;
        auto isBlackKey = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
        auto kr = rectangleForKey(i);
        g.setColour(isBlackKey ? juce::Colours::black : juce::Colours::white);
        g.fillRect(kr);
        if (midiState[i] != 0)
        {
            g.setColour(midiState[i] == 1 ? juce::Colours::orange
                                          : (isBlackKey ? juce::Colour(0x80, 0x80, 0x90)
                                                        : juce::Colour(0xC0, 0xC0, 0xD0)));
            if (i == heldNote)
                g.setColour(juce::Colours::red);

            g.fillRect(kr);
        }

        auto selZoneColor = editor->themeColor(theme::ColorMap::accent_1b);
        if (i == display->mappingView.rootKey)
        {
            g.setColour(selZoneColor);
            g.fillRect(kr);
        }

        g.setColour(juce::Colour(140, 140, 140));
        g.drawRect(kr, 0.5);
    }

    constexpr auto lastOctave = 11;
    auto font = editor->themeApplier.interRegularFor(13);
    for (int octave = 0; octave < lastOctave; ++octave)
    {
        assert(octave <= 10);
        auto r = rectangleForKey(octave * 12);
        r.setLeft(r.getX() + 1);
        r.setWidth(r.getWidth() * 12);
        // defaultMidiNoteOctaveOffset = -1 => first octave is C-2
        auto offset = -1 + sst::basic_blocks::params::ParamMetaData::defaultMidiNoteOctaveOffset;

        auto txt = fmt::format("C{}", octave + offset);
        juce::Path textPath;
        juce::GlyphArrangement glyphs;
        glyphs.addFittedText(font, txt, r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                             juce::Justification::centredLeft, 1);
        glyphs.createPath(textPath);

        g.setColour(editor->themeColor(theme::ColorMap::bg_1));
        juce::PathStrokeType strokeType(2.5f);
        g.strokePath(textPath, strokeType);
        g.setColour(editor->themeColor(theme::ColorMap::generic_content_highest));
        g.fillPath(textPath);
    }
}

void ZoneLayoutKeyboard::mouseDown(const juce::MouseEvent &e)
{
    if (!display)
        return;
    for (int i = firstMidiNote; i < lastMidiNote; ++i)
    {
        auto r = rectangleForKey(i);
        if (r.contains(e.position))
        {
            auto vy = 1.0 - 1.0 * (e.position.y - r.getY()) / (r.getHeight());
            vy = std::clamp(vy, 0., 1.);

            sendToSerialization(cmsg::NoteFromGUI({i, vy, true}));
            heldNote = i;
            if (heldNote == display->mappingView.rootKey)
            {
                moveRootKey = true;
            }
            repaint();
            return;
        }
    }
}

void ZoneLayoutKeyboard::mouseDrag(const juce::MouseEvent &e)
{
    for (int i = firstMidiNote; i < lastMidiNote; ++i)
    {
        auto r = rectangleForKey(i);
        if (r.contains(e.position))
        {
            if (i == heldNote)
            {
                // that's OK!
            }
            else
            {
                if (heldNote > 0)
                {
                    sendToSerialization(cmsg::NoteFromGUI({heldNote, 0.0f, false}));
                }
                if (moveRootKey == true)
                {
                    display->mappingView.rootKey = i;
                    display->mappingChangedFromGUI();
                }
                heldNote = i;
                auto vy = 1.0 - 1.0 * (e.position.y - r.getY()) / (r.getHeight());
                vy = std::clamp(vy, 0., 1.);

                sendToSerialization(cmsg::NoteFromGUI({i, vy, true}));
                repaint();
            }
        }
    }
}

void ZoneLayoutKeyboard::mouseUp(const juce::MouseEvent &e)
{
    if (heldNote >= 0)
    {
        sendToSerialization(cmsg::NoteFromGUI({heldNote, 0.0f, false}));
        heldNote = -1;
        moveRootKey = false;
        repaint();
        return;
    }
}

juce::Rectangle<float> ZoneLayoutKeyboard::rectangleForKey(int midiNote) const
{
    assert(lastMidiNote > firstMidiNote);
    auto normMidiNote = 1.0 * midiNote / (lastMidiNote - firstMidiNote);
    auto lb = getLocalBounds().toFloat();
    auto kw = zoomFactor * lb.getWidth() / (lastMidiNote - firstMidiNote);
    auto x = (normMidiNote - pctStart) * zoomFactor * lb.getWidth();
    auto res = lb.withWidth(kw).translated(x, 0);

    return res;
}

} // namespace scxt::ui::app::edit_screen