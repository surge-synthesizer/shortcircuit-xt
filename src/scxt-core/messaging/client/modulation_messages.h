/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_MODULATION_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_MODULATION_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/engine_traits.h"
#include "json/datamodel_traits.h"
#include "selection/selection_manager.h"
#include "voice/voice.h"
#include "detail/message_helpers.h"

namespace scxt::messaging::client
{
SERIAL_TO_CLIENT(UpdateZoneVoiceMatrixMetadata, s2c_update_zone_matrix_metadata,
                 voice::modulation::voiceMatrixMetadata_t, onZoneVoiceMatrixMetadata);

SERIAL_TO_CLIENT(UpdateZoneVoiceMatrix, s2c_update_zone_matrix,
                 voice::modulation::Matrix::RoutingTable, onZoneVoiceMatrix);

SERIAL_TO_CLIENT(UpdateGroupMatrixMetadata, s2c_update_group_matrix_metadata,
                 scxt::modulation::groupMatrixMetadata_t, onGroupMatrixMetadata);
SERIAL_TO_CLIENT(UpdateGroupMatrix, s2c_update_group_matrix, modulation::GroupMatrix::RoutingTable,
                 onGroupMatrix);

// which row, what data, and force a full update
typedef std::tuple<int, voice::modulation::Matrix::RoutingTable::Routing, bool>
    zoneRoutingRowPayload_t;
inline void doUpdateZoneRoutingRow(const zoneRoutingRowPayload_t &payload,
                                   const engine::Engine &engine, MessageController &cont)
{
    // TODO Selected Zone State
    const auto &[i, r, b] = payload;

    auto sz = engine.getSelectionManager()->currentlySelectedZones();
    if (!sz.empty())
    {
        cont.scheduleAudioThreadCallback(
            [index = i, row = r, zs = sz](auto &eng) {
                for (const auto &z : zs)
                {
                    auto &zone =
                        eng.getPatch()->getPart(z.part)->getGroup(z.group)->getZone(z.zone);
                    auto depthChange = (zone->routingTable.routes[index].depth != row.depth);
                    zone->routingTable.routes[index] = row;
                    if (!depthChange)
                    {
                        zone->onRoutingChanged();
                    }
                }
            },
            [doUpdate = b](auto &eng) {
                if (doUpdate)
                {
                    auto lz = eng.getSelectionManager()->currentLeadZone(eng);
                    if (lz.has_value())
                    {
                        eng.getSelectionManager()->sendDisplayDataForZonesBasedOnLead(
                            lz->part, lz->group, lz->zone);
                    }
                }
            });
    }
}
CLIENT_TO_SERIAL(UpdateZoneRoutingRow, c2s_update_zone_routing_row, zoneRoutingRowPayload_t,
                 doUpdateZoneRoutingRow(payload, engine, cont));

// which row, what data, and force a full update
typedef std::tuple<int, modulation::GroupMatrix::RoutingTable::Routing, bool>
    updateGroupRoutingRowPayload_t;
inline void doUpdateGroupRoutingRow(const updateGroupRoutingRowPayload_t &payload,
                                    const engine::Engine &engine, MessageController &cont)
{
    // TODO Selected Zone State
    const auto &[i, r, b] = payload;
    auto sg = engine.getSelectionManager()->currentlySelectedGroups();
    if (!sg.empty())
    {
        cont.scheduleAudioThreadCallback(
            [index = i, row = r, gs = sg](auto &eng) {
                for (const auto &z : gs)
                {
                    auto &grp = eng.getPatch()->getPart(z.part)->getGroup(z.group);
                    auto structureDiff = (row.depth == grp->routingTable.routes[index].depth &&
                                          row.active == grp->routingTable.routes[index].active);
                    grp->routingTable.routes[index] = row;
                    if (structureDiff)
                    {
                        grp->onRoutingChanged();
                        grp->rePrepareAndBindGroupMatrix();
                    }
                }
            },
            [doUpdate = b](auto &eng) {
                if (doUpdate)
                {
                    auto lg = eng.getSelectionManager()->currentLeadGroup(eng);
                    if (lg.has_value())
                        eng.getSelectionManager()->sendDisplayDataForGroupsBasedOnLead(lg->part,
                                                                                       lg->group);
                }
            });
    }
}
CLIENT_TO_SERIAL(UpdateGroupRoutingRow, c2s_update_group_routing_row,
                 updateGroupRoutingRowPayload_t, doUpdateGroupRoutingRow(payload, engine, cont));

// forzone, active, which, data
typedef std::tuple<bool, bool, int, modulation::ModulatorStorage> indexedModulatorStorageUpdate_t;
SERIAL_TO_CLIENT(UpdateGroupOrZoneLfo, s2c_update_group_or_zone_individual_modulator_storage,
                 indexedModulatorStorageUpdate_t, onGroupOrZoneModulatorStorageUpdated);

// forzone, data
typedef std::tuple<bool, modulation::MiscSourceStorage> gzMiscStorageUpdate_t;
SERIAL_TO_CLIENT(UpdateMiscModStorage, s2c_update_group_or_zone_miscmod_storage,
                 gzMiscStorageUpdate_t, onGroupOrZoneMiscModStorageUpdated);

inline void doMiscmodUpdate(const gzMiscStorageUpdate_t &payload, const engine::Engine &e,
                            messaging::MessageController &cont)
{
    auto [forzone, storage] = payload;
    if (forzone)
    {
        auto sz = e.getSelectionManager()->currentlySelectedZones();
        if (!sz.empty())
        {
            cont.scheduleAudioThreadCallback([index = 0, storage = storage, zs = sz](auto &eng) {
                for (const auto &z : zs)
                {
                    auto &zone =
                        eng.getPatch()->getPart(z.part)->getGroup(z.group)->getZone(z.zone);
                    zone->miscSourceStorage = storage;
                }
            });
        }
    }
    else
    {
        auto sg = e.getSelectionManager()->currentlySelectedGroups();
        if (!sg.empty())
        {
            cont.scheduleAudioThreadCallback([index = 0, storage = storage, gs = sg](auto &eng) {
                for (const auto &z : gs)
                {
                    auto &grp = eng.getPatch()->getPart(z.part)->getGroup(z.group);
                    grp->miscSourceStorage = storage;
                    SCLOG_IF(warnings, "Probably need to restart something here");
                }
            });
        }
    }
}
CLIENT_TO_SERIAL(UpdateMiscmodStorageForSelectedGroupOrZone,
                 c2s_update_miscmod_storage_for_groups_or_zones, gzMiscStorageUpdate_t,
                 doMiscmodUpdate(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_MODULATION_MESSAGES_H
