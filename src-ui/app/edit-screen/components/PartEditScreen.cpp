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

#include "utils.h"

#include "PartEditScreen.h"
#include "mapping-pane/MacroDisplay.h"
#include "app/SCXTEditor.h"
#include "app/shared/PartEffectsPane.h"

namespace scxt::ui::app::edit_screen
{
PartEditScreen::PartEditScreen(HasEditor *e) : HasEditor(e)
{
    macroPanel = std::make_unique<sst::jucegui::components::NamedPanel>("MACROS");
    macroDisplay = std::make_unique<MacroDisplay>(editor);
    macroPanel->addAndMakeVisible(*macroDisplay);
    addAndMakeVisible(*macroPanel);

    for (int i = 0; i < maxEffectsPerPart; ++i)
    {
        auto pep = std::make_unique<shared::PartEffectsPane<false>>(editor, this, i);

        addAndMakeVisible(*pep);
        partPanes[i] = std::move(pep);
    }
}
PartEditScreen::~PartEditScreen() = default;

void PartEditScreen::selectedPartChanged()
{
    macroDisplay->selectedPartChanged();
    for (auto &p : partPanes)
    {
        p->setSelectedPart(editor->selectedPart);
    }
}

void PartEditScreen::macroDataChanged(int part, int index)
{
    macroDisplay->macroDataChanged(part, index);
}

void PartEditScreen::resized()
{
    macroPanel->setBounds(getLocalBounds().withHeight(280));
    macroDisplay->setBounds(macroPanel->getContentArea());

    // HACK
    auto b = getLocalBounds().withTrimmedTop(280);
    auto w = shared::PartEffectsPane<false>::width;
    b = b.withWidth(w).withHeight(shared::PartEffectsPane<false>::height);
    for (int i = 0; i < maxEffectsPerPart; ++i)
    {
        partPanes[i]->setBounds(b.withX(i * w));
    }
}

void PartEditScreen::setFXSlotToType(int part, int slot, engine::AvailableBusEffects t)
{
    sendToSerialization(cmsg::SetBusEffectToType({-1, part, slot, t}));
}

void PartEditScreen::onPartEffectFullData(
    int part, int slot,
    const std::array<datamodel::pmd, engine::BusEffectStorage::maxBusEffectParams> &pmd,
    const engine::BusEffectStorage &bes)
{
    partsEffectsData[part][slot].first = pmd;
    partsEffectsData[part][slot].second = bes;
    if (partPanes[slot]->busAddressOrPart == part)
        partPanes[slot]->rebuild();
}

} // namespace scxt::ui::app::edit_screen