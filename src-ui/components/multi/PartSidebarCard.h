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

#ifndef SCXT_SRC_UI_COMPONENTS_MULTI_PARTSIDEBARCARD_H
#define SCXT_SRC_UI_COMPONENTS_MULTI_PARTSIDEBARCARD_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "sst/jucegui/components/Label.h"
#include "components/HasEditor.h"

namespace scxt::ui::multi
{
struct PartSidebarCard : juce::Component, HasEditor
{
    int part;

    static constexpr int height{88}, width{172};
    bool selfAccent{true};
    std::unique_ptr<sst::jucegui::components::Label> partLabel;
    PartSidebarCard(int p, SCXTEditor *e) : part(p), HasEditor(e)
    {
        partLabel = std::make_unique<sst::jucegui::components::Label>();
        partLabel->setText("Part " + std::to_string(part + 1));
        addAndMakeVisible(*partLabel);
        partLabel->setInterceptsMouseClicks(false, false);
    }
    void paint(juce::Graphics &g) override;

    void mouseDown(const juce::MouseEvent &e) override;

    void resized() override
    {
        partLabel->setBounds(getLocalBounds());
        partLabel->setJustification(juce::Justification::centred);
    }
};
} // namespace scxt::ui::multi
#endif // SHORTCIRCUITXT_PARTSIDEBARCARD_H
