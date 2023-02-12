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

#ifndef SCXT_UI_COMPONENTS_MULTISCREEN_H
#define SCXT_UI_COMPONENTS_MULTISCREEN_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "HasEditor.h"

namespace scxt::ui
{
namespace multi
{
struct AdsrPane;
}

struct MultiScreen : juce::Component, HasEditor
{
    struct DebugRect : public juce::Component
    {
        juce::Colour color;
        std::string label;
        DebugRect(const juce::Colour &c, const std::string &s) : color(c), label(s) {}
        void paint(juce::Graphics &g) override
        {
            g.fillAll(color);
            g.setColour(color.contrasting());
            g.drawText(label, getLocalBounds(), juce::Justification::centred);
        }
    };

    static constexpr int sideWidths = 196;
    static constexpr int edgeDistance = 6;

    static constexpr int envHeight = 160, modHeight = 160, fxHeight = 176;
    static constexpr int pad = 0;

    std::unique_ptr<juce::Component> browser, parts, mainSection, sample, fx[4], mod, mix, lfo;
    std::unique_ptr<multi::AdsrPane> eg[2];
    MultiScreen(SCXTEditor *e);
    ~MultiScreen();
    void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::black); }
    void resized() override { layout(); }

    void layout();
};
} // namespace scxt::ui
#endif // SHORTCIRCUIT_MULTISCREEN_H
