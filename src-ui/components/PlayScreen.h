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

#ifndef SCXT_SRC_UI_COMPONENTS_PLAYSCREEN_H
#define SCXT_SRC_UI_COMPONENTS_PLAYSCREEN_H

#include <juce_gui_basics/juce_gui_basics.h>

namespace scxt::ui
{
struct PlayScreen : juce::Component
{
    void paint(juce::Graphics &g)
    {
        g.fillAll(juce::Colour(0x15, 0x15, 0x15));
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(60, juce::Font::plain));
        g.drawText("Play Mode", getLocalBounds().withTrimmedBottom(40),
                   juce::Justification::centred);
        g.setFont(juce::Font(20, juce::Font::plain));
        g.drawText("Coming A Bit Less Soon", getLocalBounds().withTrimmedTop(40),
                   juce::Justification::centred);
    }
};
} // namespace scxt::ui
#endif // SHORTCIRCUITXT_PLAYSCREEN_H
