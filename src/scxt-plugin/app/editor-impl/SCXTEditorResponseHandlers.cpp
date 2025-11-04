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
#include "app/edit-screen/components/PartEditScreen.h"
#include "app/edit-screen/components/PartGroupSidebar.h"
#include "app/edit-screen/components/OutputPane.h"
#include "app/mixer-screen/MixerScreen.h"
#include "app/shared/HeaderRegion.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"
#include "app/other-screens/AboutScreen.h"
#include "app/browser-ui/BrowserPane.h"
#include "app/play-screen/PlayScreen.h"
#include "app/missing-resolution/MissingResolutionScreen.h"

namespace scxt::ui::app
{
void SCXTEditor::onGroupOrZoneEnvelopeUpdated(
    const scxt::messaging::client::adsrViewResponsePayload_t &payload)
{
    const auto &[forZone, which, active, env] = payload;
    if (forZone)
    {
        if (which == 0)
        {
            if (active)
            {
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
                editScreen->getZoneElements()->eg[1]->adsrChangedFromModel(env, which);
            }
            else
            {
                editScreen->getZoneElements()->eg[1]->adsrDeactivated();
            }
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

void SCXTEditor::onGroupOrZoneMiscModStorageUpdated(
    const scxt::messaging::client::gzMiscStorageUpdate_t &payload)
{
    const auto &[forZone, mm] = payload;
    if (forZone)
    {
        editScreen->getZoneElements()->lfo->setMiscModStorage(mm);
    }
    else
    {
        editScreen->getGroupElements()->lfo->setMiscModStorage(mm);
    }
}

void SCXTEditor::onZoneOutputInfoUpdated(const scxt::messaging::client::zoneOutputInfoUpdate_t &p)
{
    auto [active, inf] = p;
    editorDataCache.zoneOutputInfo = inf;
    editorDataCache.fireAllNotificationsFor(editorDataCache.zoneOutputInfo);
    editScreen->getZoneElements()->outPane->setActive(active);
    if (active)
    {
        editScreen->getZoneElements()->outPane->updateFromOutputInfo();
    }
}

void SCXTEditor::onGroupOutputInfoUpdated(const scxt::messaging::client::groupOutputInfoUpdate_t &p)
{
    auto [active, inf] = p;
    editorDataCache.groupOutputInfo = inf;
    editorDataCache.fireAllNotificationsFor(editorDataCache.groupOutputInfo);
    editScreen->getGroupElements()->outPane->setActive(active);
    if (active)
    {
        editScreen->getGroupElements()->outPane->updateFromOutputInfo();
    }
}

void SCXTEditor::onGroupZoneMappingSummary(const scxt::engine::Part::zoneMappingSummary_t &d)
{
    editScreen->mappingPane->setGroupZoneMappingSummary(d);

    if constexpr (scxt::log::uiStructure)
    {
        SCLOG("Updated zone mapping summary");
        for (const auto &z : d)
        {
            SCLOG("  " << z.address << " " << z.name);
        }
    }
}

void SCXTEditor::onErrorFromEngine(const scxt::messaging::client::s2cError_t &e)
{
    auto &[title, msg] = e;
    displayError(title, msg);
}

void SCXTEditor::onSelectionState(const scxt::messaging::client::selectedStateMessage_t &a)
{
    allZoneSelections = std::get<1>(a);
    allGroupSelections = std::get<3>(a);

    currentLeadZoneSelection = std::get<0>(a);
    currentLeadGroupSelection = std::get<2>(a);

    SCLOG_ONCE("TODO - this may not be needed in group multi world");
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
    if (editScreen)
    {
        editScreen->selectedPartChanged();
        editScreen->onOtherTabSelection();
    }

    repaint();
}

void SCXTEditor::onEngineStatus(const engine::Engine::EngineStatusMessage &e)
{
    engineStatus = e;
    aboutScreen->resetInfo();
    repaint();
}

void SCXTEditor::onBusOrPartEffectFullData(const scxt::messaging::client::busEffectFullData_t &d)
{
    auto busi = std::get<0>(d);
    auto parti = std::get<1>(d);
    if (busi >= 0 && mixerScreen)
    {
        auto slti = std::get<2>(d);
        const auto &bes = std::get<3>(d);
        mixerScreen->onBusEffectFullData(busi, slti, bes.first, bes.second);
    }
    if (parti >= 0 && editScreen)
    {
        auto slti = std::get<2>(d);
        const auto &bes = std::get<3>(d);
        editScreen->partEditScreen->onPartEffectFullData(parti, slti, bes.first, bes.second);
    }
}

void SCXTEditor::onBusOrPartSendData(const scxt::messaging::client::busSendData_t &d)
{
    auto busi = std::get<0>(d);
    auto parti = std::get<1>(d);
    if (busi >= 0 && mixerScreen)
    {
        const auto &busd = std::get<2>(d);
        mixerScreen->onBusSendData(busi, busd);
    }
    if (parti >= 0 && editScreen)
    {
        SCLOG("Data Send for part");
    }
}

void SCXTEditor::onBrowserRefresh(const bool)
{
    editScreen->browser->resetRoots();
    mixerScreen->browser->resetRoots();
    playScreen->browser->resetRoots();
}

void SCXTEditor::onBrowserQueueLengthRefresh(const std::pair<int32_t, int32_t> v)
{
    editScreen->browser->setIndexWorkload(v);
    mixerScreen->browser->setIndexWorkload(v);
    playScreen->browser->setIndexWorkload(v);
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
    if (editScreen && part == selectedPart)
    {
        editScreen->macroDataChanged(part, index);
    }
    playScreen->macroDataChanged(part, index);
}

void SCXTEditor::onMacroValue(const scxt::messaging::client::macroValue_t &s)
{
    const auto &[part, index, value] = s;
    macroCache[part][index].value = value;
    editScreen->mappingPane->repaint();
    editScreen->partSidebar->repaint();
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

    auto bs = queryTabSelection("browser.source");
    if (!bs.empty() && editScreen && editScreen->browser)
    {
        auto v = std::atoi(bs.c_str());
        editScreen->browser->selectPane(v);
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
    playScreen->partConfigurationChanged();

    if (editScreen && editScreen->partSidebar)
        editScreen->partSidebar->partConfigurationChanged(pt);
}

void SCXTEditor::onActivityNotification(
    const scxt::messaging::client::activityNotificationPayload_t &payload)
{
    auto [idx, msg] = payload;
    if (headerRegion)
        headerRegion->onActivityNotification(idx, msg);
}

void SCXTEditor::onMissingResolutionWorkItemList(
    const std::vector<engine::MissingResolutionWorkItem> &items)
{
    for (const auto &wi : items)
    {
        SCLOG_IF(missingResolution,
                 "Missing resolution " << (wi.isMultiUsed ? "multi-use " : "") << "work item");
        SCLOG_IF(missingResolution, "   path : " << wi.path.u8string());
        SCLOG_IF(missingResolution, "   id   : " << wi.missingID.to_string());
    }

    missingResolutionScreen->setWorkItemList(items);

    if (items.empty())
    {
        hasMissingSamples = false;
        missingResolutionScreen->setVisible(false);
    }
    else
    {
        hasMissingSamples = true;
        missingResolutionScreen->setBounds(getLocalBounds());
        missingResolutionScreen->setVisible(true);
        missingResolutionScreen->toFront(true);
    }
}

void SCXTEditor::onGroupTriggerConditions(scxt::engine::GroupTriggerConditions const &g)
{
    editScreen->partSidebar->groupTriggerConditionChanged(g);
}

void SCXTEditor::onTuningStatus(const scxt::messaging::client::tuningStatusPayload_t &t)
{
    tuningStatus = t;
}

} // namespace scxt::ui::app