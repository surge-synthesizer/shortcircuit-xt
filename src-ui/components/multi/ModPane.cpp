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

#include "ModPane.h"
#include "connectors/SCXTStyleSheetCreator.h"

namespace scxt::ui::multi
{

ModPane::ModPane(SCXTEditor *e) : sst::jucegui::components::NamedPanel(""), HasEditor(e)
{
    setCustomClass(connectors::SCXTStyleSheetCreator::ModulationTabs);

    hasHamburger = true;
    isTabbed = true;
    tabNames = {"MOD 1-6", "MOD 7-12"};

    resetTabState();

    onTabSelected = [wt = juce::Component::SafePointer(this)](int i) {
        if (wt)
            wt->label->setText(wt->tabNames[i], juce::dontSendNotification);
    };

    label = std::make_unique<juce::Label>("MAPPING", "MOD 1-6");
    label->setFont(juce::Font("Comic Sans MS", 40, juce::Font::plain));
    label->setColour(juce::Label::textColourId, juce::Colour(105, 106, 225));
    label->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*label);
}

void ModPane::resized() { label->setBounds(getContentArea()); }
} // namespace scxt::ui::multi
