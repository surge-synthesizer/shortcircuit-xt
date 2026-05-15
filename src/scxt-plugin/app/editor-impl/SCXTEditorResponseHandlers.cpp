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

#include "app/SCXTEditor.h"
#include "app/SCXTEditorReceiver.h"
#include "app/edit-screen/EditScreen.h"
#include "app/edit-screen/components/AdsrPane.h"
#include "app/edit-screen/components/LFOPane.h"
#include "app/edit-screen/components/ModPane.h"
#include "app/edit-screen/components/ProcessorPane.h"
#include "app/edit-screen/components/PartEditScreen.h"
#include "app/edit-screen/components/PartGroupSidebar.h"
#include "app/edit-screen/components/RoutingPane.h"
#include "app/mixer-screen/MixerScreen.h"
#include "app/shared/HeaderRegion.h"
#include "app/edit-screen/components/MacroMappingVariantPane.h"
#include "app/other-screens/AboutScreen.h"
#include "app/other-screens/ThemeEditor.h"
#include "app/browser-ui/BrowserPane.h"
#include "app/play-screen/PlayScreen.h"
#include "app/missing-resolution/MissingResolutionScreen.h"
#include "theme/ColorMap.h"
#include "theme/ThemeApplier.h"
#include "infrastructure/user_defaults.h"

namespace scxt::ui::app
{
void SCXTEditorReceiver::onGroupOrZoneEnvelopeUpdated(
    const scxt::messaging::client::adsrViewResponsePayload_t &payload)
{
    const auto &[forZone, which, active, env] = payload;
    if (forZone)
    {
        if (which == 0)
        {
            if (active)
            {
                editor.editScreen->getZoneElements()->eg[which]->adsrChangedFromModel(env);
            }
            else
            {
                editor.editScreen->getZoneElements()->eg[which]->adsrDeactivated();
            }
        }
        else
        {
            if (active)
            {
                editor.editScreen->getZoneElements()->eg[1]->adsrChangedFromModel(env, which);
            }
            else
            {
                editor.editScreen->getZoneElements()->eg[1]->adsrDeactivated();
            }
        }
    }
    else
    {
        if (active)
        {
            // TODO - do I want a multiScreen->onEnvelopeUpdated or just
            editor.editScreen->getGroupElements()->eg[which]->adsrChangedFromModel(env);
        }
        else
        {
            editor.editScreen->getGroupElements()->eg[which]->adsrDeactivated();
        }
    }
}

void SCXTEditorReceiver::onMappingUpdated(
    const scxt::messaging::client::mappingSelectedZoneViewResposne_t &payload)
{
    const auto &[active, m] = payload;
    if (active)
    {
        editor.editScreen->mappingPane->setActive(true);
        editor.editScreen->mappingPane->setMappingData(m);
    }
    else
    {
        // TODO
        editor.editScreen->mappingPane->setActive(false);
    }
}

void SCXTEditorReceiver::onSamplesUpdated(
    const scxt::messaging::client::sampleSelectedZoneViewResposne_t &payload)
{
    const auto &[active, s] = payload;
    if (active)
    {
        editor.editScreen->mappingPane->setActive(true);
        editor.editScreen->mappingPane->setSampleData(s);
    }
    else
    {
        editor.editScreen->mappingPane->setActive(false);
    }
}

void SCXTEditorReceiver::onStructureUpdated(const engine::Engine::pgzStructure_t &s)
{
    if (editor.editScreen && editor.editScreen->partSidebar)
    {
        editor.editScreen->partSidebar->setPartGroupZoneStructure(s);
    }
    if (editor.editScreen && editor.editScreen->mappingPane)
    {
    }
}

void SCXTEditorReceiver::onGroupOrZoneProcessorDataAndMetadata(
    const scxt::messaging::client::processorDataResponsePayload_t &d)
{
    const auto &[forZone, which, enabled, control, storage] = d;

    assert(which >= 0 && which < edit_screen::EditScreen::numProcessorDisplays);
    if (forZone)
    {
        editor.editScreen->getZoneElements()->processors[which]->setEnabled(enabled);
        editor.editScreen->getZoneElements()
            ->processors[which]
            ->setProcessorControlDescriptionAndStorage(control, storage);
        editor.editScreen->getZoneElements()->routingPane->updateFromProcessorPanes();
    }
    else
    {
        editor.editScreen->getGroupElements()->processors[which]->setEnabled(enabled);
        editor.editScreen->getGroupElements()
            ->processors[which]
            ->setProcessorControlDescriptionAndStorage(control, storage);
        editor.editScreen->getGroupElements()->routingPane->updateFromProcessorPanes();
    }
}

void SCXTEditorReceiver::onZoneVoiceMatrixMetadata(
    const scxt::voice::modulation::voiceMatrixMetadata_t &d)
{
    const auto &[active, sinf, tinf, cinf] = d;
    editor.editScreen->getZoneElements()->modPane->setActive(active);
    if (active)
    {
        editor.editScreen->getZoneElements()->modPane->matrixMetadata = d;
        editor.editScreen->getZoneElements()->modPane->rebuildMatrix();
    }
}

void SCXTEditorReceiver::onZoneVoiceMatrix(const scxt::voice::modulation::Matrix::RoutingTable &rt)
{
    assert(editor.editScreen->getZoneElements());
    assert(editor.editScreen->getZoneElements()->modPane);
    assert(editor.editScreen->getZoneElements()->modPane->isEnabled());
    editor.editScreen->getZoneElements()->modPane->routingTable = rt;
    editor.editScreen->getZoneElements()->modPane->refreshMatrix();
}

void SCXTEditorReceiver::onGroupMatrixMetadata(const scxt::modulation::groupMatrixMetadata_t &d)
{
    const auto &[active, sinf, dinf, cinf] = d;

    editor.editScreen->getGroupElements()->modPane->setActive(active);
    if (active)
    {
        editor.editScreen->getGroupElements()->modPane->matrixMetadata = d;
        editor.editScreen->getGroupElements()->modPane->rebuildMatrix();
    }
}

void SCXTEditorReceiver::onGroupMatrix(const scxt::modulation::GroupMatrix::RoutingTable &t)
{
    assert(editor.editScreen->getGroupElements()
               ->modPane->isEnabled()); // we shouldn't send a matrix to a non-enabled pane
    editor.editScreen->getGroupElements()->modPane->routingTable = t;
    editor.editScreen->getGroupElements()->modPane->refreshMatrix();
}

void SCXTEditorReceiver::onGroupOrZoneModulatorStorageUpdated(
    const scxt::messaging::client::indexedModulatorStorageUpdate_t &payload)
{
    const auto &[forZone, active, i, r] = payload;
    if (forZone)
    {
        editor.editScreen->getZoneElements()->lfo->setActive(i, active);
        editor.editScreen->getZoneElements()->lfo->setModulatorStorage(i, r);
    }
    else
    {
        editor.editScreen->getGroupElements()->lfo->setActive(i, active);
        editor.editScreen->getGroupElements()->lfo->setModulatorStorage(i, r);
    }
}

void SCXTEditorReceiver::onGroupOrZoneMiscModStorageUpdated(
    const scxt::messaging::client::gzMiscStorageUpdate_t &payload)
{
    const auto &[forZone, mm] = payload;
    if (forZone)
    {
        editor.editScreen->getZoneElements()->lfo->setMiscModStorage(mm);
    }
    else
    {
        editor.editScreen->getGroupElements()->lfo->setMiscModStorage(mm);
    }
}

void SCXTEditorReceiver::onGroupOrZoneAudioModStorageUpdated(
    const scxt::messaging::client::gzAudioModStorageUpdate_t &payload)
{
    const auto &[forZone, es] = payload;
    if (forZone)
    {
        editor.editScreen->getZoneElements()->lfo->setAudioModStorage(es);
    }
    else
    {
        editor.editScreen->getGroupElements()->lfo->setAudioModStorage(es);
    }
}

void SCXTEditorReceiver::onZoneOutputInfoUpdated(
    const scxt::messaging::client::zoneOutputInfoUpdate_t &p)
{
    auto [active, inf] = p;
    editor.editorDataCache.zoneOutputInfo = inf;
    editor.editorDataCache.fireAllNotificationsFor(editor.editorDataCache.zoneOutputInfo);
    editor.editScreen->getZoneElements()->routingPane->setActive(active);
    if (active)
    {
        editor.editScreen->getZoneElements()->routingPane->updateFromOutputInfo();
    }
}

void SCXTEditorReceiver::onGroupOutputInfoUpdated(
    const scxt::messaging::client::groupOutputInfoUpdate_t &p)
{
    auto [active, inf] = p;
    editor.editorDataCache.groupOutputInfo = inf;
    editor.editorDataCache.fireAllNotificationsFor(editor.editorDataCache.groupOutputInfo);
    editor.editScreen->getGroupElements()->routingPane->setActive(active);
    if (active)
    {
        editor.editScreen->getGroupElements()->routingPane->updateFromOutputInfo();
    }
}

void SCXTEditorReceiver::onGroupZoneMappingSummary(
    const scxt::engine::Part::zoneMappingSummary_t &d)
{
    editor.editScreen->mappingPane->setGroupZoneMappingSummary(d);

    if constexpr (scxt::log::uiStructure)
    {
        SCLOG_IF(uiStructure, "Updated zone mapping summary");
        for (const auto &z : d)
        {
            SCLOG_IF(uiStructure, "  " << z.address << " " << z.name);
        }
    }
}

void SCXTEditorReceiver::onErrorFromEngine(const scxt::messaging::client::s2cError_t &e)
{
    auto &[title, msg, source, line] = e;
    editor.displayError(title, msg);
}

void SCXTEditorReceiver::onSelectionState(const scxt::messaging::client::selectedStateMessage_t &a)
{
    editor.allZoneSelections = std::get<1>(a);
    editor.allGroupSelections = std::get<3>(a);
    auto allDisplays = std::get<4>(a);

    editor.currentLeadZoneSelection = std::get<0>(a);
    editor.currentLeadGroupSelection = std::get<2>(a);

    editor.groupsWithSelectedZones.clear();
    for (const auto &sel : editor.allZoneSelections)
    {
        if (sel.group >= 0)
            editor.groupsWithSelectedZones.insert(sel.group);
    }
    for (const auto &gz : allDisplays)
    {
        if (gz.group >= 0)
            editor.groupsWithSelectedZones.insert(gz.group);
    }

    editor.editScreen->partSidebar->editorSelectionChanged();
    editor.editScreen->mappingPane->editorSelectionChanged();

    editor.repaint();

    if constexpr (scxt::log::selection)
    {
        SCLOG_IF(selection, "Selection State Update");
        for (const auto &z : editor.allZoneSelections)
        {
            SCLOG_IF(selection, "   z : " << z);
        }
        for (const auto &z : editor.allGroupSelections)
        {
            SCLOG_IF(selection, "   g : " << z);
        }
    }
}

void SCXTEditorReceiver::onSelectedPart(const int16_t p)
{
    editor.selectedPart =
        p; // I presume I will shortly get structure messages so don't do anything else
    if (editor.editScreen)
    {
        editor.editScreen->selectedPartChanged();
        editor.editScreen->onOtherTabSelection();
    }

    editor.repaint();
}

void SCXTEditorReceiver::onEngineStatus(const engine::Engine::EngineStatusMessage &e)
{
    editor.engineStatus = e;
    editor.aboutScreen->resetInfo();
    editor.repaint();
}

void SCXTEditorReceiver::onBusOrPartEffectFullData(
    const scxt::messaging::client::busEffectFullData_t &d)
{
    auto busi = std::get<0>(d);
    auto parti = std::get<1>(d);
    if (busi >= 0 && editor.mixerScreen)
    {
        auto slti = std::get<2>(d);
        const auto &bes = std::get<3>(d);
        editor.mixerScreen->onBusEffectFullData(busi, slti, bes.first, bes.second);
    }
    if (parti >= 0 && editor.editScreen)
    {
        auto slti = std::get<2>(d);
        const auto &bes = std::get<3>(d);
        editor.editScreen->partEditScreen->onPartEffectFullData(parti, slti, bes.first, bes.second);
    }
}

void SCXTEditorReceiver::onBusOrPartSendData(const scxt::messaging::client::busSendData_t &d)
{
    auto busi = std::get<0>(d);
    auto parti = std::get<1>(d);
    if (busi >= 0 && editor.mixerScreen)
    {
        const auto &busd = std::get<2>(d);
        editor.mixerScreen->onBusSendData(busi, busd);
    }
}

void SCXTEditorReceiver::onBrowserRefresh(const bool)
{
    editor.editScreen->browser->resetRoots();
    editor.mixerScreen->browser->resetRoots();
    editor.playScreen->browser->resetRoots();
}

void SCXTEditorReceiver::onBrowserQueueLengthRefresh(const std::pair<int32_t, int32_t> v)
{
    editor.editScreen->browser->setIndexWorkload(v);
    editor.mixerScreen->browser->setIndexWorkload(v);
    editor.playScreen->browser->setIndexWorkload(v);
}

void SCXTEditorReceiver::onDebugInfoGenerated(const scxt::messaging::client::debugResponse_t &resp)
{
    for (const auto &[k, s] : resp)
    {
        SCLOG_IF(debug, k << " " << s);
    }
}

void SCXTEditorReceiver::onColormap(const std::string &json)
{
    if (json.empty())
    {
        // DES holds no edited overlay: reset to the user-prefs baseline. The
        // engine emits this unconditionally on full refresh / extra-state load.
        editor.resetColorsFromUserPreferences();
    }
    else
    {
        auto cm = theme::ColorMap::jsonToColormap(json);
        if (!cm)
            return;
        cm->hasKnobs = editor.defaultsProvider.getUserDefaultValue(
            infrastructure::DefaultKeys::showKnobs, false);
        // `jsonToColormap` does not preserve myId (serialised files are id-less
        // and it defaults to WIREFRAME). Tag as custom so the menu shows the
        // "Custom Colormap" indicator and the state machine is consistent.
        cm->myId =
            static_cast<theme::ColorMap::BuiltInColorMaps>(theme::ColorMap::CUSTOM_COLORMAP_ID);
        editor.themeApplier.recolorStylesheetWith(std::move(cm), editor.style());
    }
    editor.setStyle(editor.style());
    editor.repaint();
    if (editor.themeEditorWindow)
        editor.themeEditorWindow->rebuildFromThemeApplier();
}

void SCXTEditorReceiver::onMacroFullState(const scxt::messaging::client::macroFullState_t &s)
{
    const auto &[part, index, macro] = s;
    editor.macroCache[part][index] = macro;
    if (editor.editScreen && part == editor.selectedPart)
    {
        editor.editScreen->macroDataChanged(part, index);
    }
    editor.playScreen->macroDataChanged(part, index);
}

void SCXTEditorReceiver::onMacroValue(const scxt::messaging::client::macroValue_t &s)
{
    const auto &[part, index, value] = s;
    editor.macroCache[part][index].value = value;

    if (editor.editScreen && part == editor.selectedPart)
    {
        editor.editScreen->macroDataChanged(part, index);
    }
    editor.playScreen->macroDataChanged(part, index);

    editor.editScreen->mappingPane->repaint();
    editor.editScreen->partSidebar->repaint();
    editor.playScreen->repaint();
}

void SCXTEditorReceiver::onOtherTabSelection(
    const scxt::selection::SelectionManager::otherTabSelection_t &p)
{
    editor.otherTabSelection = p;

    auto mainScreen = editor.queryTabSelection("main_screen");
    if (mainScreen.empty())
    {
    }
    else if (mainScreen == "mixer")
    {
        editor.setActiveScreen(SCXTEditor::MIXER);
    }
    else if (mainScreen == "multi")
    {
        editor.setActiveScreen(SCXTEditor::MULTI);
    }
    else if (mainScreen == "play")
    {
        editor.setActiveScreen(SCXTEditor::PLAY);
    }
    else
    {
        SCLOG_IF(warnings, "Unknown main screen " << mainScreen);
    }

    if (editor.mixerScreen)
    {
        editor.mixerScreen->onOtherTabSelection();
    }

    if (editor.editScreen)
    {
        editor.editScreen->onOtherTabSelection();
    }

    auto bs = editor.queryTabSelection("browser.source");
    if (!bs.empty() && editor.editScreen && editor.editScreen->browser)
    {
        auto v = std::atoi(bs.c_str());
        editor.editScreen->browser->selectPane(v);
    }
}

void SCXTEditorReceiver::onPartConfiguration(
    const scxt::messaging::client::partConfigurationPayload_t &payload)
{
    const auto &[pt, c] = payload;
    assert(pt >= 0 && pt < scxt::numParts);
    editor.partConfigurations[pt] = c;
    // When I have active show/hide i will need to rewrite this i bet
    if (editor.playScreen && editor.playScreen->partSidebars[pt])
        editor.playScreen->partSidebars[pt]->resetFromEditorCache();
    editor.playScreen->partConfigurationChanged();

    if (editor.editScreen && editor.editScreen->partSidebar)
        editor.editScreen->partSidebar->partConfigurationChanged(pt);
}

void SCXTEditorReceiver::onActivityNotification(
    const scxt::messaging::client::activityNotificationPayload_t &payload)
{
    auto [idx, msg] = payload;
    if (editor.headerRegion)
    {
        editor.headerRegion->onActivityNotification(idx, msg);
    }
}

void SCXTEditorReceiver::onMissingResolutionWorkItemList(
    const std::vector<engine::MissingResolutionWorkItem> &items)
{
    for (const auto &wi : items)
    {
        SCLOG_IF(missingResolution,
                 "Missing resolution " << (wi.isMultiUsed ? "multi-use " : "") << "work item");
        SCLOG_IF(missingResolution, "   path : " << wi.path.u8string());
        SCLOG_IF(missingResolution, "   id   : " << wi.missingID.to_string());
    }

    editor.missingResolutionScreen->setWorkItemList(items);

    if (items.empty())
    {
        editor.hasMissingSamples = false;
        editor.missingResolutionScreen->setVisible(false);
    }
    else
    {
        editor.hasMissingSamples = true;
        editor.missingResolutionScreen->setBounds(editor.getLocalBounds());
        editor.missingResolutionScreen->setVisible(true);
        editor.missingResolutionScreen->toFront(true);
    }
}

void SCXTEditorReceiver::onGroupTriggerConditions(scxt::engine::GroupTriggerConditions const &g)
{
    editor.editScreen->partSidebar->groupTriggerConditionChanged(g);
}

void SCXTEditorReceiver::onTuningStatus(const scxt::messaging::client::tuningStatusPayload_t &t)
{
    editor.tuningStatus = t;
}

void SCXTEditorReceiver::onMpeTuningAwarenessFromEngine(bool a) { editor.tuningAwareMPE = a; }
void SCXTEditorReceiver::onPitchBendTuningAwarenessFromEngine(bool a)
{
    editor.tuningAwarePitchBends = a;
}

void SCXTEditorReceiver::onOmniFlavorFromEngine(std::pair<int, bool> f)
{
    editor.setupOmniApplyDefault(f.second);
    editor.setOmniFlavor(static_cast<engine::Engine::OmniFlavor>(f.first), true);
}

void SCXTEditorReceiver::onAllProcessorDescriptions(
    const std::vector<dsp::processor::ProcessorDescription> &v)
{
    editor.allProcessors = v;
}

void SCXTEditorReceiver::onClipboardType(const scxt::engine::Clipboard::ContentType &s)
{
    editor.clipboardType = s;
}

} // namespace scxt::ui::app