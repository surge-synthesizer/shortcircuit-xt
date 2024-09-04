/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#ifndef SCXT_SRC_UI_APP_EDITOR_IMPL_SCXTJUCELOOKANDFEEL_H
#define SCXT_SRC_UI_APP_EDITOR_IMPL_SCXTJUCELOOKANDFEEL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "connectors/SCXTResources.h"
#include "utils.h"

namespace scxt::ui
{
struct SCXTJuceLookAndFeel : juce::LookAndFeel_V4
{
    juce::Typeface::Ptr interMedTF;
    SCXTJuceLookAndFeel()
    {
        interMedTF = connectors::resources::loadTypeface("fonts/Inter/static/Inter-Medium.ttf");
        setColour(juce::PopupMenu::ColourIds::backgroundColourId, juce::Colour(0x15, 0x15, 0x15));
        setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId,
                  juce::Colour(0x35, 0x35, 0x45));
        setColour(juce::PopupMenu::ColourIds::highlightedTextColourId,
                  juce::Colour(0xFF, 0xFF, 0x80));
        setColour(juce::TabbedComponent::ColourIds::backgroundColourId,
                  juce::Colours::black.withAlpha(0.f));
        setColour(juce::TabbedComponent::ColourIds::outlineColourId,
                  juce::Colours::black.withAlpha(0.f));
        setColour(juce::TabbedButtonBar::ColourIds::tabOutlineColourId,
                  juce::Colours::black.withAlpha(0.f));
    }

    ~SCXTJuceLookAndFeel() {}

    juce::Font getPopupMenuFont() override
    {
#if JUCE_VERSION >= 0x080000
        return juce::Font(juce::FontOptions(interMedTF)).withHeight(13);
#else
        return juce::Font(interMedTF).withHeight(13);
#endif
    }
    void drawPopupMenuBackgroundWithOptions(juce::Graphics &g, int width, int height,
                                            const juce::PopupMenu::Options &o) override
    {
        auto background = findColour(juce::PopupMenu::backgroundColourId);

        g.fillAll(background);

        g.setColour(findColour(juce::PopupMenu::textColourId).withAlpha(0.6f));
        g.drawRect(0, 0, width, height);
    }
};
} // namespace scxt::ui
#endif // SHORTCIRCUITXT_SCXTJUCELOOKANDFEEL_H
