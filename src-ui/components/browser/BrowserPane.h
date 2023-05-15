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

#ifndef SCXT_SRC_UI_COMPONENTS_BROWSER_BROWSERPANE_H
#define SCXT_SRC_UI_COMPONENTS_BROWSER_BROWSERPANE_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/components/NamedPanel.h>

namespace scxt::ui::browser
{
struct BrowserPane : public sst::jucegui::components::NamedPanel
{
    struct CL : juce::Component
    {
        juce::Colour color;
        std::string label;
        void paint(juce::Graphics &g) override
        {
            g.setFont(juce::Font("Comic Sans MS", 40, juce::Font::plain));

            g.setColour(color);
            g.drawText(label, getLocalBounds(), juce::Justification::centred);
        }
    };
    std::unique_ptr<CL> cl;
    BrowserPane() : sst::jucegui::components::NamedPanel("Browser")
    {
        cl = std::make_unique<CL>();
        cl->color = juce::Colours::cyan;
        cl->label = "BROWSER";
        addAndMakeVisible(*cl);
        hasHamburger = true;
    }
    void resized() override { cl->setBounds(getContentArea()); }
};
} // namespace scxt::ui::browser

#endif // SHORTCIRCUITXT_BROWSERPANE_H
