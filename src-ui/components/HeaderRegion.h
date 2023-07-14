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

#ifndef SCXT_SRC_UI_COMPONENTS_HEADERREGION_H
#define SCXT_SRC_UI_COMPONENTS_HEADERREGION_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <sst/jucegui/components/ToggleButton.h>
#include <fmt/core.h>

#include <version.h>

#include "sst/jucegui/components/ToggleButtonRadioGroup.h"
#include "sst/jucegui/data/Discrete.h"
#include "widgets/ShortCircuitMenuButton.h"
#include "HasEditor.h"

namespace scxt::ui
{
struct SCXTEditor;

struct HeaderRegion : juce::Component, HasEditor
{
    std::unique_ptr<widgets::ShortCircuitMenuButton> scMenu;
    std::unique_ptr<sst::jucegui::components::ToggleButtonRadioGroup> selectedPage;
    std::unique_ptr<sst::jucegui::data::Discrete> selectedPageData;
    std::unique_ptr<juce::Component> vuMeter;

    HeaderRegion(SCXTEditor *);
    ~HeaderRegion();

    void paint(juce::Graphics &g) override
    {
#if DEBUG_VOICE_COUNT
        auto vc = fmt::format("Voices: {}", voiceCount);
        g.setColour(juce::Colours::white);
        g.drawText(vc, getLocalBounds().reduced(3, 1), juce::Justification::centred);
#endif

        return;
#if BUILD_IS_DEBUG
        g.fillAll(juce::Colours::red);
        g.setColour(juce::Colours::pink.contrasting());
        g.setFont(juce::Font("Comic Sans MS", 14, juce::Font::plain));
        g.drawText("DEBUG DEBUG DEBUG", getLocalBounds(), juce::Justification::centred);
#else
        g.fillAll(juce::Colours::pink);
        g.setColour(juce::Colours::pink.contrasting());
        g.setFont(juce::Font("Comic Sans MS", 14, juce::Font::plain));
        g.drawText("header", getLocalBounds(), juce::Justification::centred);
#endif
    }

    void resized() override;

    uint32_t voiceCount{0};
    void setVoiceCount(uint32_t vc)
    {
        if (vc != voiceCount)
        {
            voiceCount = vc;
            repaint();
        }
    }

    float vuLevel[2];
    void setVULevel(float L, float R);
};
} // namespace scxt::ui

#endif // SHORTCIRCUIT_HEADERREGION_H
