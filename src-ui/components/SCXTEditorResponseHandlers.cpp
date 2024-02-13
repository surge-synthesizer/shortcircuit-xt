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

#include "SCXTEditor.h"
#include "MultiScreen.h"
#include "multi/AdsrPane.h"
#include "multi/LFOPane.h"
#include "multi/ModPane.h"
#include "multi/ProcessorPane.h"
#include "multi/PartGroupSidebar.h"
#include "multi/OutputPane.h"
#include "MixerScreen.h"
#include "HeaderRegion.h"
#include "multi/MappingPane.h"
#include "AboutScreen.h"
#include "browser/BrowserPane.h"

namespace scxt::ui
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
            multiScreen->getZoneElements()->eg[which]->adsrChangedFromModel(env);
        }
        else
        {
            multiScreen->getZoneElements()->eg[which]->adsrDeactivated();
        }
    }
    else
    {
        if (active)
        {
            // TODO - do I want a multiScreen->onEnvelopeUpdated or just
            multiScreen->getGroupElements()->eg[which]->adsrChangedFromModel(env);
        }
        else
        {
            multiScreen->getGroupElements()->eg[which]->adsrDeactivated();
        }
    }
}

void SCXTEditor::onMappingUpdated(
    const scxt::messaging::client::mappingSelectedZoneViewResposne_t &payload)
{
    const auto &[active, m] = payload;
    if (active)
    {
        multiScreen->sample->setActive(true);
        multiScreen->sample->setMappingData(m);
    }
    else
    {
        // TODO
        multiScreen->sample->setActive(false);
    }
}

void SCXTEditor::onSamplesUpdated(
    const scxt::messaging::client::sampleSelectedZoneViewResposne_t &payload)
{
    const auto &[active, s] = payload;
    if (active)
    {
        multiScreen->sample->setActive(true);
        multiScreen->sample->setSampleData(s);
    }
    else
    {
        multiScreen->sample->setActive(false);
    }
}

void SCXTEditor::onStructureUpdated(const engine::Engine::pgzStructure_t &s)
{
    if (multiScreen && multiScreen->parts)
        multiScreen->parts->setPartGroupZoneStructure(s);
}

void SCXTEditor::onGroupOrZoneProcessorDataAndMetadata(
    const scxt::messaging::client::processorDataResponsePayload_t &d)
{
    const auto &[forZone, which, enabled, control, storage] = d;

    assert(which >= 0 && which < MultiScreen::numProcessorDisplays);
    if (forZone)
    {
        multiScreen->getZoneElements()->processors[which]->setEnabled(enabled);
        multiScreen->getZoneElements()->processors[which]->setProcessorControlDescriptionAndStorage(
            control, storage);
    }
    else
    {
        multiScreen->getGroupElements()->processors[which]->setEnabled(enabled);
        multiScreen->getGroupElements()
            ->processors[which]
            ->setProcessorControlDescriptionAndStorage(control, storage);
    }
}

void SCXTEditor::onZoneProcessorDataMismatch(
    const scxt::messaging::client::processorMismatchPayload_t &pl)
{
    const auto &[idx, leadType, leadName, otherTypes] = pl;
    multiScreen->getZoneElements()->processors[idx]->setAsMultiZone(leadType, leadName, otherTypes);
}

void SCXTEditor::onZoneVoiceMatrixMetadata(const scxt::voice::modulation::voiceMatrixMetadata_t &d)
{
    const auto &[active, sinf, tinf, cinf] = d;
    multiScreen->getZoneElements()->zoneMod()->setActive(active);
    if (active)
    {
        multiScreen->getZoneElements()->zoneMod()->matrixMetadata = d;
        multiScreen->getZoneElements()->zoneMod()->rebuildMatrix();
    }
}

void SCXTEditor::onZoneVoiceMatrix(const scxt::voice::modulation::Matrix::RoutingTable &rt)
{
    assert(multiScreen->getZoneElements());
    assert(multiScreen->getZoneElements()->zoneMod());
    assert(multiScreen->getZoneElements()->zoneMod()->isEnabled());
    multiScreen->getZoneElements()->zoneMod()->routingTable = rt;
    multiScreen->getZoneElements()->zoneMod()->refreshMatrix();
}

void SCXTEditor::onGroupMatrixMetadata(const scxt::modulation::groupModMatrixMetadata_t &d)
{
#if BADBAD
    const auto &[active, sinf, dinf, cinf] = d;
    multiScreen->getGroupElements()->groupMod()->setActive(active);
    if (active)
    {
        multiScreen->getGroupElements()->groupMod()->matrixMetadata = d;
        multiScreen->getGroupElements()->groupMod()->rebuildMatrix();
    }
#endif
}

void SCXTEditor::onGroupMatrix(const scxt::modulation::GroupModMatrix::routingTable_t &t)
{
#if BADBAD
    assert(multiScreen->getGroupElements()
               ->groupMod()
               ->isEnabled()); // we shouldn't send a matrix to a non-enabled pane
    multiScreen->getGroupElements()->groupMod()->routingTable = t;
    multiScreen->getGroupElements()->groupMod()->refreshMatrix();
#endif
}

void SCXTEditor::onGroupOrZoneModulatorStorageUpdated(
    const scxt::messaging::client::indexedModulatorStorageUpdate_t &payload)
{
    const auto &[forZone, active, i, r] = payload;
    if (forZone)
    {
        multiScreen->getZoneElements()->lfo->setActive(i, active);
        multiScreen->getZoneElements()->lfo->setModulatorStorage(i, r);
    }
    else
    {
        multiScreen->getGroupElements()->lfo->setActive(i, active);
        multiScreen->getGroupElements()->lfo->setModulatorStorage(i, r);
    }
}

void SCXTEditor::onZoneOutputInfoUpdated(const scxt::messaging::client::zoneOutputInfoUpdate_t &p)
{
    auto [active, inf] = p;
    multiScreen->getZoneElements()->zoneOut()->setActive(active);
    if (active)
    {
        multiScreen->getZoneElements()->zoneOut()->setOutputData(inf);
    }
}

void SCXTEditor::onGroupOutputInfoUpdated(const scxt::messaging::client::groupOutputInfoUpdate_t &p)
{
    auto [active, inf] = p;
    multiScreen->getGroupElements()->groupOut()->setActive(active);
    if (active)
    {
        multiScreen->getGroupElements()->groupOut()->setOutputData(inf);
    }
}

void SCXTEditor::onGroupZoneMappingSummary(const scxt::engine::Part::zoneMappingSummary_t &d)
{
    multiScreen->sample->setGroupZoneMappingSummary(d);
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

    multiScreen->parts->editorSelectionChanged();
    multiScreen->sample->editorSelectionChanged();

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
    multiScreen->browser->resetRoots();
    mixerScreen->browser->resetRoots();
}

void SCXTEditor::onDebugInfoGenerated(const scxt::messaging::client::debugResponse_t &resp)
{
    for (const auto &[k, s] : resp)
    {
        SCLOG(k << " " << s);
    }
}
} // namespace scxt::ui