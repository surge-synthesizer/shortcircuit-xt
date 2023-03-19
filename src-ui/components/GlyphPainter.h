/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_UI_COMPONENTS_GLYPHPAINTER_H
#define SCXT_SRC_UI_COMPONENTS_GLYPHPAINTER_H

#include <juce_gui_basics/juce_gui_basics.h>

namespace scxt::ui::glyph
{

struct GlyphPainter : juce::Component
{
    enum Glyph
    {
        PAN,
        VOLUME,
        TUNING
    } glyph;

    // TODO stylesheet one day
    juce::Colour colour;
    GlyphPainter(Glyph g, juce::Colour c) : glyph(g), colour(c) {}

    void paint(juce::Graphics &g) override
    {
        g.setColour(colour);
        switch (glyph)
        {
        case PAN:
            paintPanGlyph(g, getLocalBounds());
            break;
        case VOLUME:
            paintVolumeGlyph(g, getLocalBounds());
            break;
        case TUNING:
            paintTuningGlyph(g, getLocalBounds());
            break;
        }
    }
    static void paintPanGlyph(juce::Graphics &g, const juce::Rectangle<int> &into)
    {
        g.setFont(juce::Font("Comic Sans MS", 12, juce::Font::plain));
        g.setColour(juce::Colours::orange);
        g.drawFittedText("pan", into, juce::Justification::centred, 1);
    }

    static void paintVolumeGlyph(juce::Graphics &g, const juce::Rectangle<int> &into)
    {
        g.setFont(juce::Font("Comic Sans MS", 12, juce::Font::plain));
        g.setColour(juce::Colours::orange);
        g.drawFittedText("vol", into, juce::Justification::centred, 1);
    }

    static void paintTuningGlyph(juce::Graphics &g, const juce::Rectangle<int> &into)
    {
        g.setFont(juce::Font("Comic Sans MS", 12, juce::Font::plain));
        g.setColour(juce::Colours::orange);
        g.drawFittedText("tun", into, juce::Justification::centred, 1);
    }
};
} // namespace scxt::ui::glyph
#endif // SHORTCIRCUIT_GLYPHPAINTER_H
