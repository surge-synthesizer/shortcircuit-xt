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
#include "messaging/client/part_messages.h"
#include "messaging/client/mixer_messages.h"
#include "app/shared/PartEffectsPane.h"
#include "app/edit-screen/EditScreen.h"
#include "sst/jucegui/components/Label.h"
#include "sst/jucegui/components/MenuButton.h"

namespace scxt::ui::app::edit_screen
{
namespace jcmp = sst::jucegui::components;

struct PartSettingsDisplay : HasEditor, juce::Component
{
    std::unique_ptr<jcmp::Label> defaultKeySwitchLabel;
    std::unique_ptr<jcmp::MenuButton> defaultKeySwitchMenu;

    PartSettingsDisplay(SCXTEditor *e) : HasEditor(e)
    {
        defaultKeySwitchLabel = std::make_unique<jcmp::Label>();
        defaultKeySwitchLabel->setText("Default Articulation");
        defaultKeySwitchLabel->setJustification(juce::Justification::centredLeft);
        addAndMakeVisible(*defaultKeySwitchLabel);

        defaultKeySwitchMenu = std::make_unique<jcmp::MenuButton>();
        defaultKeySwitchMenu->setOnCallback([w = juce::Component::SafePointer(this)]() {
            if (w)
                w->showDefaultKeySwitchMenu();
        });
        addAndMakeVisible(*defaultKeySwitchMenu);

        refresh();
    }

    void paint(juce::Graphics &g) override
    {
        g.setFont(editor->themeApplier.interMediumFor(13));
        g.setColour(editor->themeColor(theme::ColorMap::generic_content_low));
        g.drawText("KEYSWITCHES", getLocalBounds().reduced(8, 4), juce::Justification::topLeft);
    }

    void resized() override
    {
        auto row = getLocalBounds().reduced(8, 4).withTrimmedTop(24).withHeight(16);
        defaultKeySwitchLabel->setBounds(row.withWidth(120));
        defaultKeySwitchMenu->setBounds(row.withTrimmedLeft(124).withWidth(140));
    }

    static std::string noteName(int key)
    {
        return datamodel::pmd().asMIDINote().valueToString(key).value_or(std::to_string(key));
    }

    // only a latch can be the resting articulation
    bool isLatchKey(int part, int key) const
    {
        if (key < 0 || key >= 128)
            return false;
        auto st = editor->keySwitchDisplay[part][key];
        return st == (int32_t)engine::KeySwitchDisplayState::INACTIVE ||
               st == (int32_t)engine::KeySwitchDisplayState::ACTIVE;
    }

    void refresh()
    {
        auto part = editor->selectedPart;
        if (part < 0 || part >= scxt::numParts)
            return;
        auto key = editor->partConfigurations[part].defaultKeySwitchKey;
        defaultKeySwitchMenu->setLabel(isLatchKey(part, key) ? noteName(key) : "Lowest Group");
        repaint();
    }

    void showDefaultKeySwitchMenu()
    {
        auto part = editor->selectedPart;
        if (part < 0 || part >= scxt::numParts)
            return;
        auto cur = editor->partConfigurations[part].defaultKeySwitchKey;
        auto curIsLatch = isLatchKey(part, cur);

        auto choose = [w = juce::Component::SafePointer(this), part](int16_t key) {
            return [w, part, key]() {
                if (!w)
                    return;
                auto &conf = w->editor->partConfigurations[part];
                conf.defaultKeySwitchKey = key;
                w->sendToSerialization(messaging::client::UpdatePartFullConfig({part, conf}));
                w->refresh();
            };
        };

        juce::PopupMenu p;
        p.addSectionHeader("Default Articulation");
        p.addSeparator();
        p.addItem("Lowest Group", true, !curIsLatch, choose(-1));
        bool any{false};
        for (int k = 0; k < 128; ++k)
        {
            if (!isLatchKey(part, k))
                continue;
            any = true;
            p.addItem("Switch Key " + noteName(k), true, curIsLatch && cur == k, choose(k));
        }
        if (!any)
            p.addItem("No latching keyswitches in this part", false, false, []() {});
        p.showMenuAsync(editor->defaultPopupMenuOptions(defaultKeySwitchMenu.get()));
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

void PartEditScreen::partKeySwitchStateChanged(int part)
{
    if (part == editor->selectedPart)
        partSettingsDisplay->refresh();
}

void PartEditScreen::selectedPartChanged()
{
    macroDisplay->selectedPartChanged();
    partSettingsDisplay->refresh();
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

void PartEditScreen::swapEffects(int bus1, int slot1, int bus2, int slot2,
                                 messaging::client::FXSlotDragAction act)
{
    namespace cmsg = scxt::messaging::client;
    sendToSerialization(cmsg::SwapPartFX({bus1, slot1, bus2, slot2, act}));
}

} // namespace scxt::ui::app::edit_screen