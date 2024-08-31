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

#include "app/SCXTEditor.h"
#include "app/edit-screen/EditScreen.h"
#include "app/edit-screen/components/AdsrPane.h"
#include "app/edit-screen/components/LFOPane.h"
#include "app/edit-screen/components/ModPane.h"
#include "app/edit-screen/components/ProcessorPane.h"
#include "app/edit-screen/components/PartGroupSidebar.h"
#include "app/edit-screen/components/OutputPane.h"
#include "app/mixer-screen/MixerScreen.h"
#include "app/shared/HeaderRegion.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"
#include "app/other-screens/AboutScreen.h"
#include "app/browser-ui/BrowserPane.h"
#include "app/play-screen/PlayScreen.h"

namespace scxt::ui::app
{
void SCXTEditor::onGroupOrZoneEnvelopeUpdated(
    const scxt::messaging::client::adsrViewResponsePayload_t &payload)
{
    const auto &[forZone, which, active, env] = payload;
    if (forZone)
    {
        if (active)
        {
            // TODO - do I want a multiScreen->onEnvelopeUpdated or just
            editScreen->getZoneElements()->eg[which]->adsrChangedFromModel(env);
        }
        else
        {
            editScreen->getZoneElements()->eg[which]->adsrDeactivated();
        }
    }
    else
    {
        if (active)
        {
            // TODO - do I want a multiScreen->onEnvelopeUpdated or just
            editScreen->getGroupElements()->eg[which]->adsrChangedFromModel(env);
        }
        else
        {
            editScreen->getGroupElements()->eg[which]->adsrDeactivated();
        }
    }
}

void SCXTEditor::onMappingUpdated(
    const scxt::messaging::client::mappingSelectedZoneViewResposne_t &payload)
{
    const auto &[active, m] = payload;
    if (active)
    {
        editScreen->mappingPane->setActive(true);
        editScreen->mappingPane->setMappingData(m);
    }
    else
    {
        // TODO
        editScreen->mappingPane->setActive(false);
    }
}

void SCXTEditor::onSamplesUpdated(
    const scxt::messaging::client::sampleSelectedZoneViewResposne_t &payload)
{
    const auto &[active, s] = payload;
    if (active)
    {
        editScreen->mappingPane->setActive(true);
        editScreen->mappingPane->setSampleData(s);
    }
    else
    {
        editScreen->mappingPane->setActive(false);
    }
}

void SCXTEditor::onStructureUpdated(const engine::Engine::pgzStructure_t &s)
{
    if (editScreen && editScreen->partSidebar)
    {
        editScreen->partSidebar->setPartGroupZoneStructure(s);
    }
    if (editScreen && editScreen->mappingPane)
    {
    }
}

void SCXTEditor::onGroupOrZoneProcessorDataAndMetadata(
    const scxt::messaging::client::processorDataResponsePayload_t &d)
{
    const auto &[forZone, which, enabled, control, storage] = d;

    assert(which >= 0 && which < edit_screen::EditScreen::numProcessorDisplays);
    if (forZone)
    {
        editScreen->getZoneElements()->processors[which]->setEnabled(enabled);
        editScreen->getZoneElements()->processors[which]->setProcessorControlDescriptionAndStorage(
            control, storage);
        editScreen->getZoneElements()->outPane->updateFromProcessorPanes();
    }
    else
    {
        editScreen->getGroupElements()->processors[which]->setEnabled(enabled);
        editScreen->getGroupElements()->processors[which]->setProcessorControlDescriptionAndStorage(
            control, storage);
        editScreen->getZoneElements()->outPane->updateFromProcessorPanes();
    }
}

void SCXTEditor::onZoneProcessorDataMismatch(
    const scxt::messaging::client::processorMismatchPayload_t &pl)
{
    const auto &[idx, leadType, leadName, otherTypes] = pl;
    editScreen->getZoneElements()->processors[idx]->setAsMultiZone(leadType, leadName, otherTypes);
}

void SCXTEditor::onZoneVoiceMatrixMetadata(const scxt::voice::modulation::voiceMatrixMetadata_t &d)
{
    const auto &[active, sinf, tinf, cinf] = d;
    editScreen->getZoneElements()->modPane->setActive(active);
    if (active)
    {
        editScreen->getZoneElements()->modPane->matrixMetadata = d;
        editScreen->getZoneElements()->modPane->rebuildMatrix();
    }
}

void SCXTEditor::onZoneVoiceMatrix(const scxt::voice::modulation::Matrix::RoutingTable &rt)
{
    assert(editScreen->getZoneElements());
    assert(editScreen->getZoneElements()->modPane);
    assert(editScreen->getZoneElements()->modPane->isEnabled());
    editScreen->getZoneElements()->modPane->routingTable = rt;
    editScreen->getZoneElements()->modPane->refreshMatrix();
}

void SCXTEditor::onGroupMatrixMetadata(const scxt::modulation::groupMatrixMetadata_t &d)
{
    const auto &[active, sinf, dinf, cinf] = d;

    editScreen->getGroupElements()->modPane->setActive(active);
    if (active)
    {
        editScreen->getGroupElements()->modPane->matrixMetadata = d;
        editScreen->getGroupElements()->modPane->rebuildMatrix();
    }
}

void SCXTEditor::onGroupMatrix(const scxt::modulation::GroupMatrix::RoutingTable &t)
{
    assert(editScreen->getGroupElements()
               ->modPane->isEnabled()); // we shouldn't send a matrix to a non-enabled pane
    editScreen->getGroupElements()->modPane->routingTable = t;
    editScreen->getGroupElements()->modPane->refreshMatrix();
}

void SCXTEditor::onGroupOrZoneModulatorStorageUpdated(
    const scxt::messaging::client::indexedModulatorStorageUpdate_t &payload)
{
    const auto &[forZone, active, i, r] = payload;
    if (forZone)
    {
        editScreen->getZoneElements()->lfo->setActive(i, active);
        editScreen->getZoneElements()->lfo->setModulatorStorage(i, r);
    }
    else
    {
        editScreen->getGroupElements()->lfo->setActive(i, active);
        editScreen->getGroupElements()->lfo->setModulatorStorage(i, r);
    }
}

void SCXTEditor::onZoneOutputInfoUpdated(const scxt::messaging::client::zoneOutputInfoUpdate_t &p)
{
    auto [active, inf] = p;
    editScreen->getZoneElements()->outPane->setActive(active);
    if (active)
    {
        editScreen->getZoneElements()->outPane->setOutputData(inf);
    }
}

void SCXTEditor::onGroupOutputInfoUpdated(const scxt::messaging::client::groupOutputInfoUpdate_t &p)
{
    auto [active, inf] = p;
    editScreen->getGroupElements()->outPane->setActive(active);
    if (active)
    {
        editScreen->getGroupElements()->outPane->setOutputData(inf);
    }
}

void SCXTEditor::onGroupZoneMappingSummary(const scxt::engine::Part::zoneMappingSummary_t &d)
{
    editScreen->mappingPane->setGroupZoneMappingSummary(d);

    if constexpr (scxt::log::uiStructure)
    {
        SCLOG("Updated zone mapping summary");
        for (const auto &[addr, item] : d)
        {
            SCLOG("  " << addr << " " << std::get<2>(item));
        }
    }
}

void SCXTEditor::onErrorFromEngine(const scxt::messaging::client::s2cError_t &e)
{
    auto &[title, msg] = e;
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, title, msg, "OK");
}

void SCXTEditor::onSelectionState(const scxt::messaging::client::selectedStateMessage_t &a)
{
    allZoneSelections = std::get<1>(a);
    allGroupSelections = std::get<2>(a);

    auto optLead = std::get<0>(a);
    if (optLead.has_value())
    {
        currentLeadZoneSelection = *optLead;
    }
    else
    {
        currentLeadZoneSelection = std::nullopt;
        assert(allZoneSelections.empty());
    }

    groupsWithSelectedZones.clear();
    for (const auto &sel : allZoneSelections)
    {
        if (sel.group >= 0)
            groupsWithSelectedZones.insert(sel.group);
    }

    editScreen->partSidebar->editorSelectionChanged();
    editScreen->mappingPane->editorSelectionChanged();

    repaint();

    if constexpr (scxt::log::selection)
    {
        SCLOG("Selection State Update");
        for (const auto &z : allZoneSelections)
        {
            SCLOG("   z : " << z);
        }
        for (const auto &z : allGroupSelections)
        {
            SCLOG("   g : " << z);
        }
    }
}

void SCXTEditor::onSelectedPart(const int16_t p)
{
    if constexpr (scxt::log::uiStructure)
    {
        SCLOG("Selected Part: " << p);
    }
    selectedPart = p; // I presume I will shortly get structure messages so don't do anything else
    if (editScreen && editScreen->partSidebar)
        editScreen->partSidebar->selectedPartChanged();
    if (editScreen && editScreen->mappingPane)
        editScreen->mappingPane->selectedPartChanged();
    if (editScreen)
        editScreen->onOtherTabSelection();

    repaint();
}

void SCXTEditor::onEngineStatus(const engine::Engine::EngineStatusMessage &e)
{
    engineStatus = e;
    aboutScreen->resetInfo();
    repaint();
}

void SCXTEditor::onMixerBusEffectFullData(const scxt::messaging::client::busEffectFullData_t &d)
{
    if (mixerScreen)
    {
        auto busi = std::get<0>(d);
        auto slti = std::get<1>(d);
        const auto &bes = std::get<2>(d);
        mixerScreen->onBusEffectFullData(busi, slti, bes.first, bes.second);
    }
}

void SCXTEditor::onMixerBusSendData(const scxt::messaging::client::busSendData_t &d)
{
    if (mixerScreen)
    {
        auto busi = std::get<0>(d);
        const auto &busd = std::get<1>(d);
        mixerScreen->onBusSendData(busi, busd);
    }
}

void SCXTEditor::onBrowserRefresh(const bool)
{
    editScreen->browser->resetRoots();
    mixerScreen->browser->resetRoots();
    playScreen->browser->resetRoots();
}

void SCXTEditor::onDebugInfoGenerated(const scxt::messaging::client::debugResponse_t &resp)
{
    for (const auto &[k, s] : resp)
    {
        SCLOG(k << " " << s);
    }
}

void SCXTEditor::onMacroFullState(const scxt::messaging::client::macroFullState_t &s)
{
    const auto &[part, index, macro] = s;
    macroCache[part][index] = macro;
    if (part == selectedPart)
        editScreen->mappingPane->macroDataChanged(part, index);
    playScreen->macroDataChanged(part, index);
}

void SCXTEditor::onMacroValue(const scxt::messaging::client::macroValue_t &s)
{
    const auto &[part, index, value] = s;
    macroCache[part][index].value = value;
    editScreen->mappingPane->repaint();
    playScreen->repaint();
}

void SCXTEditor::onOtherTabSelection(
    const scxt::selection::SelectionManager::otherTabSelection_t &p)
{
    otherTabSelection = p;

    auto mainScreen = queryTabSelection("main_screen");
    if (mainScreen.empty())
    {
    }
    else if (mainScreen == "mixer")
    {
        setActiveScreen(MIXER);
    }
    else if (mainScreen == "multi")
    {
        setActiveScreen(MULTI);
    }
    else if (mainScreen == "play")
    {
        setActiveScreen(PLAY);
    }
    else
    {
        SCLOG("Unknown main screen " << mainScreen);
    }

    if (mixerScreen)
    {
        mixerScreen->onOtherTabSelection();
    }

    if (editScreen)
    {
        editScreen->onOtherTabSelection();
    }
}

void SCXTEditor::onPartConfiguration(
    const scxt::messaging::client::partConfigurationPayload_t &payload)
{
    const auto &[pt, c] = payload;
    assert(pt >= 0 && pt < scxt::numParts);
    partConfigurations[pt] = c;
    // When I have active show/hide i will need to rewrite this i bet
    if (playScreen && playScreen->partSidebars[pt])
        playScreen->partSidebars[pt]->resetFromEditorCache();
    if (editScreen && editScreen->partSidebar)
        editScreen->partSidebar->partConfigurationChanged(pt);
}

void SCXTEditor::onActivityNotification(
    const scxt::messaging::client::activityNotificationPayload_t &payload)
{
    auto [idx, msg] = payload;
    // SCLOG((idx == 1 ? "Open" : (idx == 0 ? "Close" : "Update")) << " [" << msg << "]");
    SCLOG_ONCE("Update activity messages currently ignored");
}
} // namespace scxt::ui::app