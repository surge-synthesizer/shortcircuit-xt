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

#include "utils.h"

#include "PartEditScreen.h"
#include "mapping-pane/MacroDisplay.h"
#include "app/SCXTEditor.h"
#include "app/shared/PartEffectsPane.h"
#include "app/edit-screen/EditScreen.h"

namespace scxt::ui::app::edit_screen
{
struct PartSettingsDisplay : HasEditor, juce::Component
{
    PartSettingsDisplay(SCXTEditor *e) : HasEditor(e) {}
    void paint(juce::Graphics &g) override
    {
        auto ft = editor->themeApplier.interBoldFor(40);
        g.setColour(editor->themeColor(theme::ColorMap::generic_content_high));
        g.drawText("Part Settings Coming Soon", getLocalBounds(), juce::Justification::centred);
    }
};

PartEditScreen::PartEditScreen(HasEditor *e) : HasEditor(e)
{
    topPanel = std::make_unique<sst::jucegui::components::NamedPanel>("MACROS");
    topPanel->isTabbed = true;
    topPanel->tabNames = {"MACROS", "SETTINGS"};
    macroDisplay = std::make_unique<MacroDisplay>(editor);
    partSettingsDisplay = std::make_unique<PartSettingsDisplay>(editor);
    topPanel->addAndMakeVisible(*macroDisplay);
    topPanel->addChildComponent(*partSettingsDisplay);

    topPanel->onTabSelected = [this](int index) {
        if (index == 1)
        {
            partSettingsDisplay->setVisible(true);
            macroDisplay->setVisible(false);
        }
        else
        {
            partSettingsDisplay->setVisible(false);
            macroDisplay->setVisible(true);
        }
        editor->setTabSelection(editor->editScreen->tabKey("multi.part.top"),
                                std::to_string(index));
    };
    addAndMakeVisible(*topPanel);

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
    topPanel->setBounds(getLocalBounds().withHeight(280));
    macroDisplay->setBounds(topPanel->getContentArea());
    partSettingsDisplay->setBounds(topPanel->getContentArea());

    // HACK
    auto b = getLocalBounds().withTrimmedTop(280);
    auto w = shared::PartEffectsPane<false>::width;
    auto pad = (b.getWidth() - w * 4) / 4;
    b = b.withWidth(w).withHeight(shared::PartEffectsPane<false>::height);
    for (int i = 0; i < maxEffectsPerPart; ++i)
    {
        partPanes[i]->setBounds(b.withX(i * (w + pad) + pad / 2));
    }
}

void PartEditScreen::setFXSlotToType(int part, int slot, engine::AvailableBusEffects t)
{
    sendToSerialization(messaging::client::SetBusEffectToType({-1, part, slot, t}));
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

void PartEditScreen::swapEffects(int bus1, int slot1, int bus2, int slot2, bool swapVsMove)
{
    namespace cmsg = scxt::messaging::client;
    sendToSerialization(cmsg::SwapPartFX({bus1, slot1, bus2, slot2, swapVsMove}));
}

} // namespace scxt::ui::app::edit_screen