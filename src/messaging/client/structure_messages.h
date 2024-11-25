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

#ifndef SCXT_SRC_MESSAGING_CLIENT_STRUCTURE_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_STRUCTURE_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/selection_traits.h"
#include "selection/selection_manager.h"
#include "engine/engine.h"
#include "client_macros.h"

namespace scxt::messaging::client
{
/*
 * Send the full structure of all groups and zones or for a single part
 * in a single message
 */
inline void pgzSerialSide(const int &partNum, const engine::Engine &engine, MessageController &cont)
{
    serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(), cont);
}

CLIENT_SERIAL_REQUEST_RESPONSE(PartGroupZoneStructure, c2s_request_pgz_structure, int32_t,
                               s2c_send_pgz_structure, engine::Engine::pgzStructure_t,
                               pgzSerialSide(payload, engine, cont), onStructureUpdated);

SERIAL_TO_CLIENT(SendAllProcessorDescriptions, s2c_send_all_processor_descriptions,
                 std::vector<dsp::processor::ProcessorDescription>, onAllProcessorDescriptions);

/*
 * A message the client auto-sends when it registers just so we can respond
 */

inline void doRegisterClient(engine::Engine &engine, MessageController &cont)
{
    assert(cont.threadingChecker.isSerialThread());
    engine.sendMetadataToClient();
    PartGroupZoneStructure::executeOnSerialization(-1, engine, cont);
    if (engine.getSelectionManager()->currentLeadZone(engine).has_value())
    {
        // We want to re-send the action as if a lead was selected.
        // TODO - make this API less gross
        selection::SelectionManager::SelectActionContents sac{
            *(engine.getSelectionManager()->currentLeadZone(engine))};
        sac.distinct = false;
        engine.getSelectionManager()->selectAction(sac);
    }
    engine.getSelectionManager()->sendSelectedPartMacrosToClient();

    engine.sendFullRefreshToClient();
}
CLIENT_TO_SERIAL(RegisterClient, c2s_register_client, bool, doRegisterClient(engine, cont));

/*
 * A message the client auto-sends when it registers just so we can respond
 */
inline void addSample(const std::string &payload, engine::Engine &engine, MessageController &cont)
{
    assert(cont.threadingChecker.isSerialThread());
    auto p = fs::path(fs::u8path(payload));
    engine.loadSampleIntoSelectedPartAndGroup(p);
}
CLIENT_TO_SERIAL(AddSample, c2s_add_sample, std::string, addSample(payload, engine, cont);)

// sample, root, midi start end, vel start end
using addSampleWithRange_t = std::tuple<std::string, int, int, int, int, int>;
inline void addSampleWithRange(const addSampleWithRange_t &payload, engine::Engine &engine,
                               MessageController &cont)
{
    assert(cont.threadingChecker.isSerialThread());
    auto p = fs::path(fs::u8path(std::get<0>(payload)));
    auto rk = std::get<1>(payload);
    auto kr = engine::KeyboardRange(std::get<2>(payload), std::get<3>(payload));
    auto vr = engine::VelocityRange(std::get<4>(payload), std::get<5>(payload));
    engine.loadSampleIntoSelectedPartAndGroup(p, rk, kr, vr);
}
CLIENT_TO_SERIAL(AddSampleWithRange, c2s_add_sample_with_range, addSampleWithRange_t,
                 addSampleWithRange(payload, engine, cont);)

// sample, part, group, wone, sampleid
using addSampleInZone_t = std::tuple<std::string, int, int, int, int>;
inline void addSampleInZone(const addSampleInZone_t &payload, engine::Engine &engine,
                            MessageController &cont)
{
    assert(cont.threadingChecker.isSerialThread());
    auto path = fs::path(fs::u8path(std::get<0>(payload)));
    auto part{std::get<1>(payload)};
    auto group{std::get<2>(payload)};
    auto zone{std::get<3>(payload)};
    auto sampleID{std::get<4>(payload)};

    engine.loadSampleIntoZone(path, part, group, zone, sampleID);
}
CLIENT_TO_SERIAL(AddSampleInZone, c2s_add_sample_in_zone, addSampleInZone_t,
                 addSampleInZone(payload, engine, cont);)

inline void createGroupIn(int partNumber, engine::Engine &engine, MessageController &cont)
{
    if (partNumber < 0 || partNumber > numParts)
        partNumber = 0;
    SCLOG_IF(groupZoneMutation, "Creating group in part " << partNumber);
    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [p = partNumber](auto &e) { e.getPatch()->getPart(p)->addGroup(); },
        [p = partNumber](auto &engine) {
            SCLOG_IF(groupZoneMutation, "Responding with part group zone structure in " << p);
            serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(),
                                      *(engine.getMessageController()));

            serializationSendToClient(s2c_send_selected_group_zone_mapping_summary,
                                      engine.getPatch()->getPart(p)->getZoneMappingSummary(),
                                      *(engine.getMessageController()));
            if (engine.getSelectionManager()->currentlySelectedZones().empty())
            {
                SCLOG_IF(selection, "Empty selection when creating group in " << p);
                // ooof what to do
                int32_t g = engine.getPatch()->getPart(p)->getGroups().size() - 1;
                engine.getSelectionManager()->selectAction(
                    selection::SelectionManager::SelectActionContents(p, g, -1, true, true, true));
            }
        });
}
CLIENT_TO_SERIAL(CreateGroup, c2s_create_group, int, createGroupIn(payload, engine, cont));

inline void removeZone(const selection::SelectionManager::ZoneAddress &a, engine::Engine &engine,
                       MessageController &cont)
{
    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [s = a](auto &e) {
            auto &zoneO = e.getPatch()->getPart(s.part)->getGroup(s.group)->getZone(s.zone);
            e.terminateVoicesForZone(*zoneO);
            auto zid = zoneO->id;
            auto z = e.getPatch()->getPart(s.part)->getGroup(s.group)->removeZone(zid);
            auto zoneToFree = z.release();

            messaging::audio::AudioToSerialization a2s;
            a2s.id = audio::a2s_delete_this_pointer;
            a2s.payloadType = audio::AudioToSerialization::TO_BE_DELETED;
            a2s.payload.delThis.ptr = zoneToFree;
            a2s.payload.delThis.type = audio::AudioToSerialization::ToBeDeleted::engine_Zone;
            e.getMessageController()->sendAudioToSerialization(a2s);
        },
        [t = a](auto &engine) {
            engine.getSampleManager()->purgeUnreferencedSamples();
            engine.getSelectionManager()->guaranteeConsistencyAfterDeletes(engine, true, t);
            serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(),
                                      *(engine.getMessageController()));
            serializationSendToClient(s2c_send_selected_group_zone_mapping_summary,
                                      engine.getPatch()->getPart(t.part)->getZoneMappingSummary(),
                                      *(engine.getMessageController()));
        });
}
CLIENT_TO_SERIAL(DeleteZone, c2s_delete_zone, selection::SelectionManager::ZoneAddress,
                 removeZone(payload, engine, cont));

inline void removeSelectedZones(const bool &, engine::Engine &engine, MessageController &cont)
{
    auto part = engine.getSelectionManager()->selectedPart;

    auto zs = engine.getSelectionManager()->allSelectedZones[part];

    if (zs.empty())
        return;

    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [sz = zs](auto &e) {
            // I know this allocates and deallocates on audio thread
            std::vector<std::tuple<int, int, ZoneID>> ids;
            for (auto s : sz)
            {
                auto &zoneO = e.getPatch()->getPart(s.part)->getGroup(s.group)->getZone(s.zone);
                e.terminateVoicesForZone(*zoneO);

                ids.push_back(
                    {s.part, s.group,
                     e.getPatch()->getPart(s.part)->getGroup(s.group)->getZone(s.zone)->id});
            }
            for (auto [p, g, zid] : ids)
            {
                auto z = e.getPatch()->getPart(p)->getGroup(g)->removeZone(zid);
                auto zoneToFree = z.release();

                messaging::audio::AudioToSerialization a2s;
                a2s.id = audio::a2s_delete_this_pointer;
                a2s.payloadType = audio::AudioToSerialization::TO_BE_DELETED;
                a2s.payload.delThis.ptr = zoneToFree;
                a2s.payload.delThis.type = audio::AudioToSerialization::ToBeDeleted::engine_Zone;
                e.getMessageController()->sendAudioToSerialization(a2s);
            }
        },
        [t = part](auto &engine) {
            engine.getSampleManager()->purgeUnreferencedSamples();
            engine.getSelectionManager()->guaranteeConsistencyAfterDeletes(engine, true,
                                                                           {t, -1, -1});

            serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(),
                                      *(engine.getMessageController()));
            serializationSendToClient(s2c_send_selected_group_zone_mapping_summary,
                                      engine.getPatch()->getPart(t)->getZoneMappingSummary(),
                                      *(engine.getMessageController()));
        });
}
CLIENT_TO_SERIAL(DeleteAllSelectedZones, c2s_delete_selected_zones, bool,
                 removeSelectedZones(payload, engine, cont));

inline void removeGroup(const selection::SelectionManager::ZoneAddress &a, engine::Engine &engine,
                        MessageController &cont)
{
    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [s = a](auto &e) {
            if (e.getPatch()->getPart(s.part)->getGroups().size() < s.group)
                return;

            auto &groupO = e.getPatch()->getPart(s.part)->getGroup(s.group);
            e.terminateVoicesForGroup(*groupO);

            auto gid = e.getPatch()->getPart(s.part)->getGroup(s.group)->id;
            auto groupToFree = e.getPatch()->getPart(s.part)->removeGroup(gid).release();

            messaging::audio::AudioToSerialization a2s;
            a2s.id = audio::a2s_delete_this_pointer;
            a2s.payloadType = audio::AudioToSerialization::TO_BE_DELETED;
            a2s.payload.delThis.ptr = groupToFree;
            a2s.payload.delThis.type = audio::AudioToSerialization::ToBeDeleted::engine_Group;
            e.getMessageController()->sendAudioToSerialization(a2s);
        },
        [t = a](auto &engine) {
            engine.getSampleManager()->purgeUnreferencedSamples();
            engine.getSelectionManager()->guaranteeConsistencyAfterDeletes(engine, false, t);

            serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(),
                                      *(engine.getMessageController()));
            serializationSendToClient(s2c_send_selected_group_zone_mapping_summary,
                                      engine.getPatch()->getPart(t.part)->getZoneMappingSummary(),
                                      *(engine.getMessageController()));
        });
}
CLIENT_TO_SERIAL(DeleteGroup, c2s_delete_group, selection::SelectionManager::ZoneAddress,
                 removeGroup(payload, engine, cont));

inline void clearPart(const int p, engine::Engine &engine, MessageController &cont)
{
    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [pt = p](auto &e) {
            auto &part = e.getPatch()->getPart(pt);
            while (!part->getGroups().empty())
            {
                auto &groupO = part->getGroup(0);
                e.terminateVoicesForGroup(*groupO);
                auto gid = groupO->id;
                auto g = part->removeGroup(gid);

                auto groupToFree = g.release();

                messaging::audio::AudioToSerialization a2s;
                a2s.id = audio::a2s_delete_this_pointer;
                a2s.payloadType = audio::AudioToSerialization::TO_BE_DELETED;
                a2s.payload.delThis.ptr = groupToFree;
                a2s.payload.delThis.type = audio::AudioToSerialization::ToBeDeleted::engine_Group;
                e.getMessageController()->sendAudioToSerialization(a2s);
            }
        },
        [pt = p](auto &engine) {
            engine.getSampleManager()->purgeUnreferencedSamples();
            engine.getSelectionManager()->guaranteeConsistencyAfterDeletes(engine, false,
                                                                           {pt, -1, -1});

            serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(),
                                      *(engine.getMessageController()));
            serializationSendToClient(s2c_send_selected_group_zone_mapping_summary,
                                      engine.getPatch()->getPart(pt)->getZoneMappingSummary(),
                                      *(engine.getMessageController()));
        });
}
CLIENT_TO_SERIAL(ClearPart, c2s_clear_part, int, clearPart(payload, engine, cont));

using zoneAddressFromTo_t =
    std::pair<selection::SelectionManager::ZoneAddress, selection::SelectionManager::ZoneAddress>;
inline void moveZoneFromTo(const zoneAddressFromTo_t &payload, engine::Engine &engine,
                           MessageController &cont)
{
    auto &src = payload.first;
    auto &tgt = payload.second;

    assert(src.part == tgt.part);

    auto nad = engine.getPatch()->getPart(tgt.part)->getGroup(tgt.group)->getZones().size();

    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [s = src, t = tgt](auto &e) {
            if (s.group == t.group)
            {
                if (t.zone == -1)
                {
                    // You've moved an item onto its own goroup. So that's a noop
                }
                else
                {
                    // Its a zone swap
                    auto &group = e.getPatch()->getPart(s.part)->getGroup(s.group);
                    auto &zoneS = group->getZone(s.zone);
                    auto &zoneT = group->getZone(t.zone);
                    e.terminateVoicesForZone(*zoneS);
                    e.terminateVoicesForZone(*zoneT);
                    group->swapZonesByIndex(s.zone, t.zone);
                }
            }
            else
            {
                auto &zoneO = e.getPatch()->getPart(s.part)->getGroup(s.group)->getZone(s.zone);
                e.terminateVoicesForZone(*zoneO);

                auto zid = zoneO->id;
                auto z = e.getPatch()->getPart(s.part)->getGroup(s.group)->removeZone(zid);
                if (z)
                    e.getPatch()->getPart(t.part)->getGroup(t.group)->addZone(z);
            }
        },
        [nad, s = src, t = tgt](auto &engine) {
            if (s.group != t.group)
            {
                // Swapped groups. We almost definitely need to select in the new group
                auto tc = t;
                tc.zone = nad;
                auto act = selection::SelectionManager::SelectActionContents(tc, true, true, true);
                engine.getSelectionManager()->selectAction(act);
            }
            serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(),
                                      *(engine.getMessageController()));
            serializationSendToClient(s2c_send_selected_group_zone_mapping_summary,
                                      engine.getPatch()->getPart(t.part)->getZoneMappingSummary(),
                                      *(engine.getMessageController()));
        });
}
CLIENT_TO_SERIAL(MoveZoneFromTo, c2s_move_zone, zoneAddressFromTo_t,
                 moveZoneFromTo(payload, engine, cont));

inline void doActivateNextPart(messaging::MessageController &cont)
{
    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [](auto &e) {
            // We should really do this on audio thread but test it here quickly
            for (auto &pt : *(e.getPatch()))
            {
                if (!pt->configuration.active)
                {
                    pt->clearGroups();
                    pt->configuration.active = true;
                    break;
                }
            }
        },
        [](const auto &e) { e.sendFullRefreshToClient(); });
}
CLIENT_TO_SERIAL(ActivateNextPart, c2s_activate_next_part, bool, doActivateNextPart(cont));

inline void doDeactivatePart(int part, messaging::MessageController &cont)
{
    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [part](auto &e) {
            e.getPatch()->getPart(part)->configuration.active = false;
            bool anySel{false};
            for (auto &pt : *(e.getPatch()))
            {
                if (pt->configuration.active)
                    anySel = true;
            }
            if (!anySel)
            {
                e.getPatch()->getPart(0)->clearGroups();
                e.getPatch()->getPart(0)->configuration.active = true;
            }
        },
        [part](const auto &e) {
            if (e.getSelectionManager()->selectedPart == part)
            {
                int tpt{0}, spt{-1};
                for (auto &pt : *(e.getPatch()))
                {
                    if (pt->configuration.active && spt < 0)
                        spt = tpt;
                    tpt++;
                }
                if (spt >= 0)
                {
                    e.getSelectionManager()->selectPart(spt);
                }
            }
            e.sendFullRefreshToClient();
        });
}
CLIENT_TO_SERIAL(DeactivatePart, c2s_deactivate_part, int32_t, doDeactivatePart(payload, cont));

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_STRUCTURE_MESSAGES_H
