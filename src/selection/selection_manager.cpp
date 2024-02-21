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

#define DEBUG_SELECTION 0

namespace scxt::selection
{
namespace cms = messaging::client;

void SelectionManager::sendClientDataForLeadSelectionState()
{
    auto [p, g, z] = leadZone;

    // Check if I deserialized a valid zone and adjust if not
    if (p >= 0)
    {
        selectedPart = p;
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

    if (p >= 0 && g >= 0)
    {
        serializationSendToClient(cms::s2c_send_selected_group_zone_mapping_summary,
                                  engine.getPatch()->getPart(p)->getZoneMappingSummary(),
                                  *(engine.getMessageController()));
    }

    if (allSelectedGroups.size() > 1)
    {
        SCLOG_UNIMPL("Multi-group selection to do");
    }
    else if (allSelectedGroups.size() == 1)
    {
        auto [gp, gg, gz] = *(allSelectedGroups.begin());
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

void SelectionManager::multiSelectAction(const std::vector<SelectActionContents> &v)
{
    for (const auto &z : v)
        adjustInternalStateForAction(z);
    guaranteeSelectedLead();
    sendClientDataForLeadSelectionState();
    sendSelectedZonesToClient();
    debugDumpSelectionState();
}

void SelectionManager::selectAction(
    const scxt::selection::SelectionManager::SelectActionContents &z)
{
    /* A wildcard zone single select means select all in group */
    if (z.forZone && z.isContiguous)
    {
        auto zf = (ZoneAddress)z;
        auto zt = z.contiguousFrom;
        if (zt < zf)
        {
            SCLOG("Swapping ZF and ZT");
            std::swap(zf, zt);
        }
        if (zf.part != zt.part)
        {
            SCLOG("ERROR: Can't do contiguous selection across parts");
            return;
        }
        auto gs = zf.group;
        auto ge = zt.group;
        std::vector<SelectActionContents> res;

        for (auto gi = gs; gi <= ge; ++gi)
        {
            const auto &grp = engine.getPatch()->getPart(zf.part)->getGroup(gi);
            auto sz = ((gi == gs && zf.zone >= 0) ? zf.zone : 0);
            auto ez = ((gi == ge && zt.zone >= 0) ? zt.zone : grp->getZones().size() - 1);
            for (auto zi = sz; zi <= ez; ++zi)
            {
                SelectActionContents se;
                se.part = zt.part;
                se.group = gi;
                se.zone = zi;
                se.selecting = true;
                se.distinct = false;
                se.selectingAsLead = false;

                res.push_back(se);
            }

            multiSelectAction(res);
        }
        return;
    }
    if (z.forZone && z.zone == -1)
    {
        auto za = (ZoneAddress)z;
        if (!za.isInWithPartials(engine) || za.group == -1)
        {
            SCLOG("ERROR: Zone not consistent with engine");
            return;
        }

        const auto &g = engine.getPatch()->getPart(za.part)->getGroup(za.group);

        std::vector<SelectActionContents> res;
        int idx{0};
        for (const auto &zn : *g)
        {
            auto rc = z;
            rc.zone = idx++;
            res.push_back(rc);
        }
        multiSelectAction(res);
        return;
    }
    adjustInternalStateForAction(z);
    guaranteeSelectedLead();
    sendClientDataForLeadSelectionState();
    sendSelectedZonesToClient();
    debugDumpSelectionState();
}

void SelectionManager::adjustInternalStateForAction(
    const scxt::selection::SelectionManager::SelectActionContents &z)
{
#if DEBUG_SELECTION
    SCLOG_WFUNC(z);
#endif
    auto za = (ZoneAddress)z;

    if (z.forZone)
    {
        // OK so basically theres a few cases
        if (!z.selecting)
        {
            allSelectedZones.erase(za);
        }
        else
        {
            // So we are selecting
            if (z.distinct)
            {
                allSelectedZones.clear();
                allSelectedZones.insert(za);
                leadZone = za;
            }
            else
            {
                allSelectedZones.insert(za);
                if (z.selectingAsLead)
                    leadZone = za;
            }
        }
    }
    else
    {
        SCLOG("Group Naive Single Selection Selection " << za);
        allSelectedGroups.clear();
        allSelectedGroups.insert(za);
        leadGroup = za;
    }
}

void SelectionManager::guaranteeConsistencyAfterDeletes(const engine::Engine &engine)
{
    bool morphed{false};
    auto zit = allSelectedZones.begin();
    while (zit != allSelectedZones.end())
    {
        if (!zit->isIn(engine))
        {
            morphed = true;
            zit = allSelectedZones.erase(zit);
        }
        else
        {
            zit++;
        }
    }

    auto git = allSelectedGroups.begin();
    while (git != allSelectedGroups.end())
    {
        if (!git->isIn(engine))
        {
            morphed = true;
            git = allSelectedGroups.erase(git);
        }
        else
        {
            git++;
        }
    }

    if (morphed)
    {
        guaranteeSelectedLead();
        sendClientDataForLeadSelectionState();
        sendSelectedZonesToClient();
        debugDumpSelectionState();
    }
}

void SelectionManager::guaranteeSelectedLead()
{
    // Now at the end of this the lead zone could not be selected
    if (allSelectedZones.find(leadZone) == allSelectedZones.end())
    {
        if (allSelectedZones.empty() && leadZone.part >= 0)
        {
            // Oh what to do here. Well reject it.
            SCLOG_WFUNC("Be careful - we are promoting " << SCD(leadZone));
            allSelectedZones.insert(leadZone);
        }
        else if (!allSelectedZones.empty())
        {
            leadZone = *(allSelectedZones.begin());
        }
        else
        {
            leadZone = {};
        }
    }

    // Still at group single so far
    assert(allSelectedGroups.size() < 2);
    if (allSelectedGroups.find(leadGroup) == allSelectedGroups.end())
    {
        if (!allSelectedGroups.empty())
        {
            leadGroup = *(allSelectedGroups.begin());
        }
        else
        {
            leadGroup = {};
        }
    }
}

void SelectionManager::debugDumpSelectionState()
{
#if DEBUG_SELECTION
    SCLOG("---------------------------");
    SCLOG("ZONES: Selected=" << allSelectedZones.size());
    SCLOG("   LEAD ZONE : " << leadZone);
    for (const auto &z : allSelectedZones)
        SCLOG("      Z Selected : " << z);

    SCLOG("GROUPS: Selected=" << allSelectedGroups.size());
    SCLOG("   LEAD GROUP : " << leadGroup);
    for (const auto &z : allSelectedGroups)
        SCLOG("      G Selected : " << z);
    SCLOG("---------------------------");
#endif
}

void SelectionManager::sendSelectedZonesToClient()
{
#if DEBUG_SELECTION
    SCLOG("Sending selection data to client");
    debugDumpSelectionState();
#endif

    serializationSendToClient(
        cms::s2c_send_selection_state,
        cms::selectedStateMessage_t{leadZone, allSelectedZones, allSelectedGroups},
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
    if (allSelectedZones.empty())
    {
        return {0, 0};
    }

    // For now pick the highest group in the lowest part
    auto pt = 17;
    auto gp = 0;
    for (const auto &s : allSelectedZones)
    {
        if (s.part >= 0)
            pt = std::min(pt, s.part);
    }
    if (pt == 17)
        pt = 0;

    for (const auto &s : allSelectedZones)
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
                              cms::SampleSelectedZoneView::s2c_payload_t{true, zp->sampleData},
                              *(engine.getMessageController()));
    serializationSendToClient(
        cms::s2c_update_group_or_zone_adsr_view,
        cms::AdsrGroupOrZoneUpdate::s2c_payload_t{true, 0, true, zp->egStorage[0]},
        *(engine.getMessageController()));
    serializationSendToClient(
        cms::s2c_update_group_or_zone_adsr_view,
        cms::AdsrGroupOrZoneUpdate::s2c_payload_t{true, 1, true, zp->egStorage[1]},
        *(engine.getMessageController()));

    for (int i = 0; i < engine::lfosPerZone; ++i)
    {
        serializationSendToClient(
            cms::s2c_update_group_or_zone_individual_modulator_storage,
            cms::indexedModulatorStorageUpdate_t{true, true, i, zp->modulatorStorage[i]},
            *(engine.getMessageController()));
    }

    /*
     * Processors, Output and ModMatrix have multi-select merge rules which are different
     */
    for (int i = 0; i < engine::processorCount; ++i)
    {
        auto typ = processorTypesForSelectedZones(i);
        if (typ.size() == 1)
        {
            serializationSendToClient(
                cms::s2c_respond_single_processor_metadata_and_data,
                cms::ProcessorMetadataAndData::s2c_payload_t{
                    true, i, true, zp->processorDescription[i], zp->processorStorage[i]},
                *(engine.getMessageController()));
        }
        else
        {
            // just inter-process not perma streamed so safe as int
            std::set<int32_t> ptInt;
            for (const auto &t : typ)
                ptInt.insert((int32_t)t);
            serializationSendToClient(cms::s2c_notify_mismatched_processors_for_zone,
                                      cms::ProcessorsMismatched::s2c_payload_t{
                                          i, (int32_t)zp->processorDescription[i].type,
                                          zp->processorDescription[i].typeDisplayName, ptInt},
                                      *(engine.getMessageController()));
        }
    }

    configureAndSendZoneModMatrixMetadata(p, g, z);

    serializationSendToClient(cms::s2c_update_zone_output_info,
                              cms::zoneOutputInfoUpdate_t{true, zp->outputInfo},
                              *(engine.getMessageController()));

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

    for (int i = 0; i < engine::Group::egPerGroup; ++i)
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
                              modulation::getGroupModMatrixMetadata(*g),
                              *(engine.getMessageController()));
    serializationSendToClient(cms::s2c_update_group_matrix, rt, *(engine.getMessageController()));
}

void SelectionManager::sendDisplayDataForNoGroupSelected()
{
    for (int i = 0; i < engine::Group::egPerGroup; ++i)
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

std::set<dsp::processor::ProcessorType> SelectionManager::processorTypesForSelectedZones(int pidx)
{
    std::set<dsp::processor::ProcessorType> res;

    const auto &pt = engine.getPatch();
    for (const auto &z : allSelectedZones)
    {
        if (z.isIn(engine))
        {
            res.insert(pt->getPart(z.part)
                           ->getGroup(z.group)
                           ->getZone(z.zone)
                           ->processorStorage[pidx]
                           .type);
        }
    }
    return res;
}

void SelectionManager::copyZoneProcessorLeadToAll(int which)
{
    auto lz = currentLeadZone(engine);
    if (!lz.has_value())
        return;
    if (allSelectedZones.size() < 2)
        return;

    auto &cont = engine.getMessageController();
    cont->scheduleAudioThreadCallback(
        [asz = allSelectedZones, from = *lz, which](auto &e) {
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
    mat.prepare(zp->routingTable);
    scxt::voice::modulation::MatrixEndpoints ep{nullptr};
    ep.bindTargetBaseValues(mat, *zp);
    for (auto &r : zp->routingTable.routes)
    {
        if (r.target.has_value())
        {
            if (scxt::voice::modulation::MatrixConfig::isTargetModMatrixDepth(*r.target))
            {
                auto rti =
                    scxt::voice::modulation::MatrixConfig::getTargetModMatrixElement(*r.target);
                r.extraPayload.targetBaseValue = zp->routingTable.routes[rti].depth;
                r.extraPayload.targetMetadata =
                    datamodel::pmd().asPercent().withName("Row " + std::to_string(rti + 1));
            }
            else
            {
                r.extraPayload.targetMetadata = mat.activeTargetsToPMD.at(*r.target);
                r.extraPayload.targetBaseValue = mat.activeTargetsToBaseValue.at(*r.target);
            }
        }
        else
        {
            r.extraPayload.targetBaseValue = 0;
            r.extraPayload.targetMetadata = datamodel::pmd().asPercent().withName("Target");
        }
    }

    if (allSelectedZones.size() == 1)
    {
        for (auto &row : zp->routingTable.routes)
            row.extraPayload.selConsistent = true;

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
            row.extraPayload.selConsistent = true;
        for (const auto &sz : allSelectedZones)
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
                row.extraPayload.selConsistent = row.extraPayload.selConsistent && same;
            }
        }

        // TODO: Make this zone or group based
        serializationSendToClient(cms::s2c_update_zone_matrix_metadata,
                                  voice::modulation::getVoiceMatrixMetadata(*zp),
                                  *(engine.getMessageController()));

        serializationSendToClient(cms::s2c_update_zone_matrix, rt,
                                  *(engine.getMessageController()));
    }
}
} // namespace scxt::selection