/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

void SelectionManager::sendClientDataForSelectionState()
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
    if (z >= 0 && g >= 0 && p >= 0)
    {
        // TODO: The 'full zone' becomes a single function obviously
        const auto &zp = engine.getPatch()->getPart(p)->getGroup(g)->getZone(z);
        serializationSendToClient(cms::s2c_respond_zone_mapping,
                                  cms::MappingSelectedZoneView::s2c_payload_t{true, zp->mapping},
                                  *(engine.getMessageController()));
        serializationSendToClient(cms::s2c_respond_zone_samples,
                                  cms::SampleSelectedZoneView::s2c_payload_t{true, zp->sampleData},
                                  *(engine.getMessageController()));
        serializationSendToClient(cms::s2c_respond_zone_adsr_view,
                                  cms::AdsrSelectedZoneView::s2c_payload_t{0, true, zp->aegStorage},
                                  *(engine.getMessageController()));
        serializationSendToClient(cms::s2c_respond_zone_adsr_view,
                                  cms::AdsrSelectedZoneView::s2c_payload_t{1, true, zp->eg2Storage},
                                  *(engine.getMessageController()));

        for (int i = 0; i < engine::processorsPerZone; ++i)
        {
            serializationSendToClient(
                cms::s2c_respond_single_processor_metadata_and_data,
                cms::ProcessorMetadataAndData::s2c_payload_t{i, true, zp->processorDescription[i],
                                                             zp->processorStorage[i]},
                *(engine.getMessageController()));
        }

        for (int i = 0; i < engine::lfosPerZone; ++i)
        {
            serializationSendToClient(cms::s2c_update_zone_individual_lfo,
                                      cms::indexedLfoUpdate_t{true, i, zp->lfoStorage[i]},
                                      *(engine.getMessageController()));
        }
        serializationSendToClient(cms::s2c_update_zone_voice_matrix_metadata,
                                  modulation::getVoiceModMatrixMetadata(*zp),
                                  *(engine.getMessageController()));
        serializationSendToClient(cms::s2c_update_zone_voice_matrix, zp->routingTable,
                                  *(engine.getMessageController()));
    }
    else
    {
        serializationSendToClient(cms::s2c_respond_zone_mapping,
                                  cms::MappingSelectedZoneView::s2c_payload_t{false, {}},
                                  *(engine.getMessageController()));

        serializationSendToClient(cms::s2c_respond_zone_samples,
                                  cms::SampleSelectedZoneView::s2c_payload_t{false, {}},
                                  *(engine.getMessageController()));
        serializationSendToClient(cms::s2c_respond_zone_adsr_view,
                                  cms::AdsrSelectedZoneView::s2c_payload_t{0, false, {}},
                                  *(engine.getMessageController()));
        serializationSendToClient(cms::s2c_respond_zone_adsr_view,
                                  cms::AdsrSelectedZoneView::s2c_payload_t{1, false, {}},
                                  *(engine.getMessageController()));
        for (int i = 0; i < engine::processorsPerZone; ++i)
        {
            serializationSendToClient(
                cms::s2c_respond_single_processor_metadata_and_data,
                cms::ProcessorMetadataAndData::s2c_payload_t{i, false, {}, {}},
                *(engine.getMessageController()));
        }
        serializationSendToClient(cms::s2c_update_zone_voice_matrix_metadata,
                                  modulation::voiceModMatrixMetadata_t{false, {}, {}, {}},
                                  *(engine.getMessageController()));
    }
}

void SelectionManager::multiSelectAction(const std::vector<SelectActionContents> &v)
{
    for (const auto &z : v)
        adjustInternalStateForAction(z);
    guaranteeSelectedLead();
    sendClientDataForSelectionState();
    sendSelectedZonesToClient();
    debugDumpSelectionState();
}

void SelectionManager::selectAction(
    const scxt::selection::SelectionManager::SelectActionContents &z)
{
    adjustInternalStateForAction(z);
    guaranteeSelectedLead();
    sendClientDataForSelectionState();
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
    // OK so basically theres a few cases
    if (!z.selecting)
    {
        SCLOG("All selected zones: removing " << za)
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
        else
        {
            leadZone = *(allSelectedZones.begin());
        }
    }
}

void SelectionManager::debugDumpSelectionState()
{
#if DEBUG_SELECTION
    SCLOG("---------------------------");
    SCLOG(SCD(leadZone));
    SCLOG("All Selected Zones");
    for (const auto &s : allSelectedZones)
        SCLOG("    - " << s);
    SCLOG("---------------------------");
#endif
}

void SelectionManager::sendSelectedZonesToClient()
{
#if DEBUG_SELECTION
    SCLOG("Sending selection data to client");
#endif
    serializationSendToClient(cms::s2c_send_selection_state,
                              cms::selectedStateMessage_t{leadZone, allSelectedZones},
                              *(engine.getMessageController()));
}

bool SelectionManager::ZoneAddress::isIn(const engine::Engine &e)
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
} // namespace scxt::selection