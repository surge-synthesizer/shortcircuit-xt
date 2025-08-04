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

#include "selection_manager.h"
#include <iostream>
#include "engine/engine.h"
#include "engine/patch.h"
#include "messaging/client/selection_messages.h"
#include "messaging/messaging.h"
#include "messaging/client/client_serial.h"
#include "messaging/client/processor_messages.h"

namespace scxt::selection
{
namespace cms = messaging::client;

void SelectionManager::sendClientDataForLeadSelectionState()
{
    if constexpr (scxt::log::selection)
    {
        SCLOG("Sending full selection data to client for part=" << selectedPart);
    }
    serializationSendToClient(cms::s2c_send_selected_part, selectedPart,
                              *(engine.getMessageController()));

    auto [p, g, z] = leadZone[selectedPart];
    assert(p < 0 || p == selectedPart);

    // Check if I deserialized a valid zone and adjust if not
    if (p >= 0)
    {
        if (p >= numParts)
        {
            p = -1;
            g = -1;
            z = -1;
        }
        else if (g >= 0)
        {
            if (g >= engine.getPatch()->getPart(p)->getGroups().size())
            {
                g = -1;
                z = -1;
            }
            else if (z >= 0)
            {
                if (z >= engine.getPatch()->getPart(p)->getGroup(g)->getZones().size())
                {
                    z = -1;
                }
            }
        }
    }

    if (p >= 0)
    {
        serializationSendToClient(cms::s2c_send_selected_group_zone_mapping_summary,
                                  engine.getPatch()->getPart(p)->getZoneMappingSummary(),
                                  *(engine.getMessageController()));
    }

    if (allSelectedGroups[selectedPart].size() > 1)
    {
        SCLOG_UNIMPL_ONCE("Multi-group selection to do");
    }
    else if (allSelectedGroups[selectedPart].size() == 1)
    {
        auto [gp, gg, gz] = *(allSelectedGroups[selectedPart].begin());
        sendDisplayDataForSingleGroup(gp, gg);
    }
    else
    {
        sendDisplayDataForNoGroupSelected();
    }

    if (z >= 0 && g >= 0 && p >= 0)
    {
        sendDisplayDataForZonesBasedOnLead(p, g, z);
    }
    else
    {
        sendDisplayDataForNoZoneSelected();
    }
}

void SelectionManager::applySelectActions(const std::vector<SelectActionContents> &v)
{
    auto r = transformSelectionActions(v);
    for (const auto &z : r)
    {
        if (!z.addr().isInWithPartials(engine))
        {
            // TODO - error message
            continue;
        }
        adjustInternalStateForAction(z);
    }
    guaranteeSelectedLead();
    sendClientDataForLeadSelectionState();
    sendSelectedZonesToClient();
    debugDumpSelectionState();
}

std::vector<SelectionManager::SelectActionContents>
SelectionManager::transformSelectionActions(const std::vector<SelectActionContents> &inEls)
{
    if (inEls.size() > 1)
    {
        return inEls;
    }
    auto &sing = inEls[0];

    // single select a group selects all zones
    if ((sing.forZone && sing.zone == -1 && sing.group >= 0) ||
        (!sing.forZone && sing.group >= 0 && sing.zone == -1))
    {
        auto z = sing;
        const auto &g = engine.getPatch()->getPart(z.part)->getGroup(z.group);

        std::vector<SelectActionContents> res;
        int idx{0};
        for (const auto &zn : *g)
        {
            auto rc = z;
            rc.forZone = true;
            rc.distinct = (idx == 0) && z.distinct;
            rc.selectingAsLead = (idx == 0);
            rc.selecting = true;
            rc.zone = idx++;
            res.push_back(rc);
        }
        if (!z.forZone)
            res.push_back(z);
        return res;
    }

    return inEls;
}

void SelectionManager::selectPart(int16_t part)
{
    selectedPart = part;

    if constexpr (scxt::log::selection)
    {
        SCLOG("SELECT PART " << part);
    }

    guaranteeSelectedLead();
    sendClientDataForLeadSelectionState();
    sendSelectedZonesToClient();
    sendSelectedPartMacrosToClient();
    debugDumpSelectionState();
}

void SelectionManager::adjustInternalStateForAction(
    const scxt::selection::SelectionManager::SelectActionContents &z)
{
    if constexpr (scxt::log::selection)
    {
        SCLOG_WFUNC(z);
    }
    auto za = (ZoneAddress)z;

    if (z.forZone)
    {
        // OK so basically theres a few cases
        if (!z.selecting)
        {
            allSelectedZones[selectedPart].erase(za);
        }
        else
        {
            // So we are selecting
            if (z.distinct)
            {
                allSelectedZones[selectedPart].clear();
                allSelectedZones[selectedPart].insert(za);
                leadZone[selectedPart] = za;
            }
            else
            {
                allSelectedZones[selectedPart].insert(za);
                if (z.selectingAsLead)
                    leadZone[selectedPart] = za;
            }
        }

        /*
         * Under zone selection we want the following
         * - Selected groups is the union of zone groups
         * - lead group is the group of lead zone
         */
        std::set<int32_t> gset;
        allSelectedGroups[selectedPart].clear();
        for (const auto &z : allSelectedZones[selectedPart])
        {
            gset.insert(z.group);
        }
        for (auto g : gset)
        {
            allSelectedGroups[selectedPart].insert({selectedPart, g, -1});
        }
        leadGroup[selectedPart] = {selectedPart, leadZone[selectedPart].group, -1};
        SCLOG_IF(selection,
                 "AllSelectedGroups[" << selectedPart << "] " << gset.size() << " groups");
        SCLOG_IF(selection, "LeadGroup[" << selectedPart << "] " << leadGroup[selectedPart]);
    }
    else
    {
        SCLOG_IF(selection, "Group Naive Single Selection Selection " << za);
        allSelectedGroups[selectedPart].clear();
        allSelectedGroups[selectedPart].insert(za);
        leadGroup[selectedPart] = za;
    }
}

void SelectionManager::sendGroupZoneMappingForSelectedPart()
{
    if (selectedPart < 0 || selectedPart >= numParts)
        return;

    serializationSendToClient(cms::s2c_send_selected_group_zone_mapping_summary,
                              engine.getPatch()->getPart(selectedPart)->getZoneMappingSummary(),
                              *(engine.getMessageController()));
}

void SelectionManager::guaranteeConsistencyAfterDeletes(const engine::Engine &engine,
                                                        bool zoneDeleted,
                                                        const ZoneAddress &whatIDeleted)
{
    bool morphed{false};
    auto zit = allSelectedZones[selectedPart].begin();
    while (zit != allSelectedZones[selectedPart].end())
    {
        if (!zit->isIn(engine))
        {
            SCLOG_IF(groupZoneMutation,
                     "Removing zone " << *zit << " from selection on group delete");
            if (leadZone[selectedPart] == *zit)
            {
                SCLOG_IF(groupZoneMutation, "Lead zone removed from selection");
                leadZone[selectedPart] = {};
            }
            morphed = true;
            zit = allSelectedZones[selectedPart].erase(zit);
        }
        else
        {
            zit++;
        }
    }

    auto git = allSelectedGroups[selectedPart].begin();
    while (git != allSelectedGroups[selectedPart].end())
    {
        if (!git->isIn(engine))
        {
            morphed = true;
            SCLOG_IF(groupZoneMutation,
                     "Removing group " << *git << " from selection on group delete");
            if (leadGroup[selectedPart] == *git)
            {
                SCLOG_IF(groupZoneMutation, "Lead group removed from selection");
                leadGroup[selectedPart] = {};
            }

            git = allSelectedGroups[selectedPart].erase(git);
        }
        else
        {
            git++;
        }
    }

    if (morphed)
    {
        auto [p, g, z] = whatIDeleted;
        guaranteeSelectedLeadSomewhereIn(p, (zoneDeleted ? g : -1), -1);
        sendClientDataForLeadSelectionState();
        sendSelectedZonesToClient();
        debugDumpSelectionState();
    }
}

void SelectionManager::guaranteeSelectedLeadSomewhereIn(int part, int group, int zone)
{
    guaranteeSelectedLead();
    if (leadZone[selectedPart] == ZoneAddress())
    {
        const auto &partO = engine.getPatch()->getPart(part);
        bool gotOne{false};
        if (!partO->getGroups().empty())
        {
            if (group >= 0 && group < partO->getGroups().size())
            {
                auto &groupO = partO->getGroup(group);
                if (!groupO->getZones().empty())
                {
                    leadZone[selectedPart] = {part, group, 0};
                    leadGroup[selectedPart] = {part, group, -1};
                    gotOne = true;
                }
            }
            if (!gotOne)
            {
                auto gs = partO->getGroups().size();
                for (int i = 0; i < gs && !gotOne; ++i)
                {
                    if (!partO->getGroup(i)->getZones().empty())
                    {
                        leadZone[selectedPart] = {part, i, 0};
                        leadGroup[selectedPart] = {part, group, -1};
                        gotOne = true;
                    }
                }
            }
            if (gotOne)
            {
                guaranteeSelectedLead();
            }
        }
    }
}

void SelectionManager::guaranteeSelectedLead()
{
    // Now at the end of this the lead zone could not be selected
    if (allSelectedZones[selectedPart].find(leadZone[selectedPart]) ==
        allSelectedZones[selectedPart].end())
    {
        // Nothing is selected. So we need to pick the first zone of the first group
        if (allSelectedZones[selectedPart].empty())
        {
            auto &pt = engine.getPatch()->getPart(selectedPart);
            if (pt->getGroups().empty())
            {
                leadZone = {};
            }
            else
            {
                auto &gr = pt->getGroup(0);
                if (gr->getZones().empty())
                {
                    leadZone[selectedPart] = {};
                }
                else
                {
                    leadZone[selectedPart] = {selectedPart, 0, 0};
                    SCLOG_IF(selection, "Lead zone is first zone of first group");
                }
            }
        }
        else if (!allSelectedZones[selectedPart].empty())
        {
            leadZone[selectedPart] = *(allSelectedZones[selectedPart].begin());
        }
    }

    // Still at group single so far
    if (allSelectedGroups[selectedPart].find(leadGroup[selectedPart]) ==
        allSelectedGroups[selectedPart].end())
    {
        SCLOG_IF(selection, "Promoting lead group in guaranteeSelectedLead");
        leadGroup[selectedPart] = {selectedPart, leadZone[selectedPart].group, -1};
        if (allSelectedGroups[selectedPart].find(leadGroup[selectedPart]) ==
                allSelectedGroups[selectedPart].end() &&
            leadGroup[selectedPart].group >= 0)
        {
            SCLOG_IF(selection, "Adding zone lead to AllSelectedGroups");
            allSelectedGroups[selectedPart].insert(leadGroup[selectedPart]);
        }
    }
}

void SelectionManager::debugDumpSelectionState()
{
    if constexpr (scxt::log::selection)
    {
        SCLOG("---------------------------");
        SCLOG("PART: " << selectedPart);
        SCLOG("ZONES: Selected=" << allSelectedZones[selectedPart].size());
        SCLOG("   LEAD ZONE : " << leadZone[selectedPart]);
        for (const auto &z : allSelectedZones[selectedPart])
            SCLOG("      Z Selected : " << z);

        SCLOG("GROUPS: Selected=" << allSelectedGroups[selectedPart].size());
        SCLOG("   LEAD GROUP : " << leadGroup[selectedPart]);
        for (const auto &z : allSelectedGroups[selectedPart])
            SCLOG("      G Selected : " << z);
        SCLOG("---------------------------");
    }
}

void SelectionManager::sendSelectedZonesToClient()
{
    if constexpr (scxt::log::selection)
    {
        SCLOG("Sending selected zones to client");
        debugDumpSelectionState();
    }

    serializationSendToClient(
        cms::s2c_send_selection_state,
        cms::selectedStateMessage_t{leadZone[selectedPart], allSelectedZones[selectedPart],
                                    leadGroup[selectedPart], allSelectedGroups[selectedPart]},
        *(engine.getMessageController()));
}

bool SelectionManager::ZoneAddress::isIn(const engine::Engine &e) const
{
    if (part < 0 || part > numParts)
        return false;
    const auto &p = e.getPatch()->getPart(part);
    if (group < 0 || group >= p->getGroups().size())
        return false;
    const auto &g = p->getGroup(group);
    if (zone < 0 || zone >= g->getZones().size())
        return false;
    return true;
}

bool SelectionManager::ZoneAddress::isInWithPartials(const engine::Engine &e) const
{
    if (part < 0 || part > numParts)
        return false;
    if (group == -1)
        return true;
    const auto &p = e.getPatch()->getPart(part);
    if (group < 0 || group >= p->getGroups().size())
        return false;
    if (zone == -1)
        return true;
    const auto &g = p->getGroup(group);
    if (zone < 0 || zone >= g->getZones().size())
        return false;
    return true;
}

std::pair<int, int> SelectionManager::bestPartGroupForNewSample(const engine::Engine &e)
{
    if (currentLeadZone(e).has_value())
    {
        auto [sp, sg, sz] = *currentLeadZone(e);
        if (sp < 0)
            sp = 0;
        if (sg < 0)
            sg = 0;

        return {sp, sg};
    }
    if (allSelectedZones[selectedPart].empty())
    {
        return {selectedPart, 0};
    }

    // For now pick the highest group in the selected part
    auto pt = selectedPart;
    auto gp = 0;

    for (const auto &s : allSelectedZones[selectedPart])
    {
        if (s.part == pt && s.group >= 0)
            gp = std::max(gp, s.group);
    }

    return {pt, gp};
}

void SelectionManager::sendDisplayDataForZonesBasedOnLead(int p, int g, int z)
{
    /*
     * The samples, mapping, ADSR, and LFO are simple functions of the lead zone.
     * Since there's no structural differences (yet - LFO types may bifurcate this)
     * send the value of the lead and let it edit for ADSR and LFO; and the mapping is
     * just the mapping for the lead zone
     */
    const auto &zp = engine.getPatch()->getPart(p)->getGroup(g)->getZone(z);
    serializationSendToClient(cms::s2c_respond_zone_mapping,
                              cms::MappingSelectedZoneView::s2c_payload_t{true, zp->mapping},
                              *(engine.getMessageController()));
    serializationSendToClient(cms::s2c_respond_zone_samples,
                              cms::SampleSelectedZoneView::s2c_payload_t{true, zp->variantData},
                              *(engine.getMessageController()));

    for (auto i = 0U; i < scxt::egsPerZone; ++i)
    {
        serializationSendToClient(
            cms::s2c_update_group_or_zone_adsr_view,
            cms::AdsrGroupOrZoneUpdate::s2c_payload_t{true, i, true, zp->egStorage[i]},
            *(engine.getMessageController()));
    }

    for (int i = 0; i < engine::lfosPerZone; ++i)
    {
        auto rsh = zp->modulatorStorage[i].modulatorShape;
        auto con = true;
        if (allSelectedZones[selectedPart].size() > 1)
        {
            for (const auto &sz : allSelectedZones[selectedPart])
            {
                const auto &zsh = engine.getPatch()
                                      ->getPart(sz.part)
                                      ->getGroup(sz.group)
                                      ->getZone(sz.zone)
                                      ->modulatorStorage[i]
                                      .modulatorShape;
                if (zsh != rsh)
                {
                    con = false;
                    break;
                }
            }
        }

        zp->modulatorStorage[i].modulatorConsistent = con;
        serializationSendToClient(
            cms::s2c_update_group_or_zone_individual_modulator_storage,
            cms::indexedModulatorStorageUpdate_t{true, true, i, zp->modulatorStorage[i]},
            *(engine.getMessageController()));
    }
    serializationSendToClient(cms::s2c_update_group_or_zone_miscmod_storage,
                              cms::gzMiscStorageUpdate_t{true, zp->miscSourceStorage},
                              *(engine.getMessageController()));

    /*
     * Processors, Output and ModMatrix have multi-select merge rules which are different
     */
    for (int i = 0; i < engine::processorCount; ++i)
    {
        if (acrossSelectionConsistency(true, PROCESSOR_TYPE, i))
        {
            serializationSendToClient(
                cms::s2c_respond_single_processor_metadata_and_data,
                cms::ProcessorMetadataAndData::s2c_payload_t{
                    true, i, true, zp->processorDescription[i], zp->processorStorage[i]},
                *(engine.getMessageController()));
        }
        else
        {
            serializationSendToClient(cms::s2c_notify_mismatched_processors_for_zone,
                                      cms::ProcessorsMismatched::s2c_payload_t{
                                          i, (int32_t)zp->processorDescription[i].type,
                                          zp->processorDescription[i].typeDisplayName},
                                      *(engine.getMessageController()));
        }
    }

    configureAndSendZoneModMatrixMetadata(p, g, z);

    // Update across selections here to see if the routing is consistent
    auto rt = zp->outputInfo.procRouting;
    auto con = true;
    if (allSelectedZones[selectedPart].size() > 1)
    {
        for (const auto &sz : allSelectedZones[selectedPart])
        {
            const auto &zpr = engine.getPatch()
                                  ->getPart(sz.part)
                                  ->getGroup(sz.group)
                                  ->getZone(sz.zone)
                                  ->outputInfo.procRouting;

            if (zpr != rt)
            {
                con = false;
                break;
            }
        }
    }
    zp->outputInfo.procRoutingConsistent = con;

    serializationSendToClient(cms::s2c_update_zone_output_info,
                              cms::zoneOutputInfoUpdate_t{true, zp->outputInfo},
                              *(engine.getMessageController()));
}

void SelectionManager::sendDisplayDataForNoZoneSelected()
{
    serializationSendToClient(cms::s2c_respond_zone_mapping,
                              cms::MappingSelectedZoneView::s2c_payload_t{false, {}},
                              *(engine.getMessageController()));

    serializationSendToClient(cms::s2c_respond_zone_samples,
                              cms::SampleSelectedZoneView::s2c_payload_t{false, {}},
                              *(engine.getMessageController()));
    serializationSendToClient(cms::s2c_update_group_or_zone_adsr_view,
                              cms::AdsrGroupOrZoneUpdate::s2c_payload_t{true, 0, false, {}},
                              *(engine.getMessageController()));
    serializationSendToClient(cms::s2c_update_group_or_zone_adsr_view,
                              cms::AdsrGroupOrZoneUpdate::s2c_payload_t{true, 1, false, {}},
                              *(engine.getMessageController()));
    for (int i = 0; i < engine::processorCount; ++i)
    {
        serializationSendToClient(
            cms::s2c_respond_single_processor_metadata_and_data,
            cms::ProcessorMetadataAndData::s2c_payload_t{true, i, false, {}, {}},
            *(engine.getMessageController()));
    }

    serializationSendToClient(cms::s2c_update_zone_matrix_metadata,
                              voice::modulation::voiceMatrixMetadata_t{false, {}, {}, {}},
                              *(engine.getMessageController()));

    serializationSendToClient(cms::s2c_update_zone_output_info,
                              cms::zoneOutputInfoUpdate_t{false, {}},
                              *(engine.getMessageController()));
}

void SelectionManager::sendDisplayDataForSingleGroup(int part, int group)
{
    const auto &g = engine.getPatch()->getPart(part)->getGroup(group);
    serializationSendToClient(cms::s2c_update_group_output_info,
                              cms::groupOutputInfoUpdate_t{true, g->outputInfo},
                              *(engine.getMessageController()));

    for (int i = 0; i < scxt::egsPerGroup; ++i)
    {
        serializationSendToClient(
            cms::s2c_update_group_or_zone_adsr_view,
            cms::AdsrGroupOrZoneUpdate::s2c_payload_t{false, i, true, g->gegStorage[i]},
            *(engine.getMessageController()));
    }

    for (int i = 0; i < engine::lfosPerZone; ++i)
    {
        serializationSendToClient(
            cms::s2c_update_group_or_zone_individual_modulator_storage,
            cms::indexedModulatorStorageUpdate_t{false, true, i, g->modulatorStorage[i]},
            *(engine.getMessageController()));
    }
    serializationSendToClient(cms::s2c_update_group_or_zone_miscmod_storage,
                              cms::gzMiscStorageUpdate_t{false, g->miscSourceStorage},
                              *(engine.getMessageController()));

    for (int i = 0; i < engine::processorCount; ++i)
    {
        serializationSendToClient(
            cms::s2c_respond_single_processor_metadata_and_data,
            cms::ProcessorMetadataAndData::s2c_payload_t{false, i, true, g->processorDescription[i],
                                                         g->processorStorage[i]},
            *(engine.getMessageController()));
    }

    auto &rt = g->routingTable;
    serializationSendToClient(cms::s2c_update_group_matrix_metadata,
                              modulation::getGroupMatrixMetadata(*g),
                              *(engine.getMessageController()));
    serializationSendToClient(cms::s2c_update_group_matrix, rt, *(engine.getMessageController()));

    serializationSendToClient(cms::s2c_send_group_trigger_conditions, g->triggerConditions,
                              *(engine.getMessageController()));
}

void SelectionManager::sendDisplayDataForNoGroupSelected()
{
    for (int i = 0; i < scxt::egsPerGroup; ++i)
    {
        serializationSendToClient(cms::s2c_update_group_or_zone_adsr_view,
                                  cms::AdsrGroupOrZoneUpdate::s2c_payload_t{false, i, false, {}},
                                  *(engine.getMessageController()));
    }

    for (int i = 0; i < engine::processorCount; ++i)
    {
        serializationSendToClient(
            cms::s2c_respond_single_processor_metadata_and_data,
            cms::ProcessorMetadataAndData::s2c_payload_t{false, i, false, {}, {}},
            *(engine.getMessageController()));
    }
}

void SelectionManager::copyZoneProcessorLeadToAll(int which)
{
    auto lz = currentLeadZone(engine);
    if (!lz.has_value())
        return;
    if (allSelectedZones[selectedPart].size() < 2)
        return;

    auto &cont = engine.getMessageController();
    cont->scheduleAudioThreadCallback(
        [asz = allSelectedZones[selectedPart], from = *lz, which](auto &e) {
            const auto &fz =
                e.getPatch()->getPart(from.part)->getGroup(from.group)->getZone(from.zone);
            auto ftype = fz->processorStorage[which].type;
            for (const auto &sz : asz)
            {
                if (sz == from)
                {
                    // Nothintg to do
                }
                else
                {
                    const auto &tz =
                        e.getPatch()->getPart(sz.part)->getGroup(sz.group)->getZone(sz.zone);

                    tz->setProcessorType(which, ftype);
                    tz->processorStorage[which] = fz->processorStorage[which];
                }
            }
        },
        [this](auto &e) {
            auto lz = currentLeadZone(e);
            if (lz.has_value())
                sendDisplayDataForZonesBasedOnLead(lz->part, lz->group, lz->zone);
        });
}

void SelectionManager::configureAndSendZoneModMatrixMetadata(int p, int g, int z)
{
    const auto &zp = engine.getPatch()->getPart(p)->getGroup(g)->getZone(z);

    // I really need to document this but basicaly this fixes the PMDs on the targets on the
    // routing table. Or maybe even make it a function on zone (that's probably better).
    scxt::voice::modulation::Matrix mat;
    mat.forUIMode = true;
    mat.prepare(zp->routingTable, engine.getSampleRate(), blockSize);
    scxt::voice::modulation::MatrixEndpoints ep{nullptr};
    ep.bindTargetBaseValues(mat, *zp);
    for (auto &r : zp->routingTable.routes)
    {
        r.extraPayload = scxt::voice::modulation::MatrixConfig::RoutingExtraPayload();

        if (r.target.has_value())
        {
            if (scxt::voice::modulation::MatrixConfig::isTargetModMatrixDepth(*r.target))
            {
                auto rti =
                    scxt::voice::modulation::MatrixConfig::getTargetModMatrixElement(*r.target);
                r.extraPayload->targetBaseValue = zp->routingTable.routes[rti].depth;
                r.extraPayload->targetMetadata =
                    datamodel::pmd().asPercent().withName("Row " + std::to_string(rti + 1));
            }
            else
            {
                r.extraPayload->targetMetadata = mat.activeTargetsToPMD.at(*r.target);
                r.extraPayload->targetBaseValue = mat.activeTargetsToBaseValue.at(*r.target);
            }
        }
        else
        {
            r.extraPayload->targetBaseValue = 0;
            r.extraPayload->targetMetadata = datamodel::pmd().asPercent().withName("Target");
        }
    }

    if (allSelectedZones[selectedPart].size() == 1)
    {
        for (auto &row : zp->routingTable.routes)
            row.extraPayload->selConsistent = true;

        serializationSendToClient(cms::s2c_update_zone_matrix_metadata,
                                  voice::modulation::getVoiceMatrixMetadata(*zp),
                                  *(engine.getMessageController()));
        serializationSendToClient(cms::s2c_update_zone_matrix, zp->routingTable,
                                  *(engine.getMessageController()));
    }
    else
    {
        // OK so is the routing table consistent over multiple zones
        auto &rt = zp->routingTable;
        for (auto &row : rt.routes)
            row.extraPayload->selConsistent = true;
        for (const auto &sz : allSelectedZones[selectedPart])
        {
            const auto &szrt = engine.getPatch()
                                   ->getPart(sz.part)
                                   ->getGroup(sz.group)
                                   ->getZone(sz.zone)
                                   ->routingTable;

            for (const auto &[ridx, row] : sst::cpputils::enumerate(rt.routes))
            {
                auto &srow = szrt.routes[ridx];
                auto same = (srow.active == row.active && srow.source == row.source &&
                             srow.sourceVia == row.sourceVia && srow.curve == row.curve &&
                             srow.target == row.target);
                row.extraPayload->selConsistent = row.extraPayload->selConsistent && same;
            }
        }

        // TODO: Make this zone or group based
        serializationSendToClient(cms::s2c_update_zone_matrix_metadata,
                                  voice::modulation::getVoiceMatrixMetadata(*zp),
                                  *(engine.getMessageController()));

        serializationSendToClient(cms::s2c_update_zone_matrix, rt,
                                  *(engine.getMessageController()));
    }

    for (auto &r : zp->routingTable.routes)
    {
        r.extraPayload = std::nullopt;
    }
}

void SelectionManager::sendSelectedPartMacrosToClient()
{
    const auto &part = engine.getPatch()->getPart(selectedPart);
    for (int i = 0; i < macrosPerPart; ++i)
        serializationSendToClient(cms::s2c_update_macro_full_state,
                                  cms::macroFullState_t{selectedPart, i, part->macros[i]},
                                  *(engine.getMessageController()));
}

void SelectionManager::sendOtherTabsSelectionToClient()
{
    serializationSendToClient(cms::s2c_send_othertab_selection, otherTabSelection,
                              *(engine.getMessageController()));
}

void SelectionManager::clearAllSelections()
{
    for (auto &s : allSelectedZones)
        s.clear();
    for (auto &s : allSelectedGroups)
        s.clear();
    for (auto &z : leadZone)
        z = {};
    for (auto &g : leadGroup)
        g = {};
    selectedPart = 0;
}

bool SelectionManager::acrossSelectionConsistency(bool forZone, ConsistencyCheck whichCheck,
                                                  int index)
{
    if (forZone)
    {
        auto &lza = leadZone[selectedPart];
        if (!lza.isInWithPartials(engine))
            return true;

        const auto &lz =
            engine.getPatch()->getPart(lza.part)->getGroup(lza.group)->getZone(lza.zone);

        for (auto &s : allSelectedZones[selectedPart])
        {
            if (!s.isInWithPartials(engine))
                continue;
            const auto &it = engine.getPatch()->getPart(s.part)->getGroup(s.group)->getZone(s.zone);
            switch (whichCheck)
            {
            case PROCESSOR_TYPE:
                if (lz->processorStorage[index].type != it->processorStorage[index].type)
                    return false;
                break;
            case MATRIX_ROW:
                break;
            }
        }
    }
    else
    {
        SCLOG_UNIMPL("Group branch");
    }
    return true;
}

} // namespace scxt::selection