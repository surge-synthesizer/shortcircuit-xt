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
    serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(partNum),
                              cont);
}

CLIENT_SERIAL_REQUEST_RESPONSE(PartGroupZoneStructure, c2s_request_pgz_structure, int32_t,
                               s2c_send_pgz_structure, engine::Engine::pgzStructure_t,
                               pgzSerialSide(payload, engine, cont), onStructureUpdated);

SERIAL_TO_CLIENT(SendAllProcessorDescriptions, s2c_send_all_processor_descriptions,
                 std::vector<dsp::processor::ProcessorDescription>, onAllProcessorDescriptions);

/*
 * A message the client auto-sends when it registers just so we can respond
 */

inline void onRegister(engine::Engine &engine, MessageController &cont)
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
}
CLIENT_TO_SERIAL(OnRegister, c2s_on_register, bool, onRegister(engine, cont));

/*
 * A message the client auto-sends when it registers just so we can respond
 */
inline void addSample(const std::string &payload, engine::Engine &engine, MessageController &cont)
{
    assert(cont.threadingChecker.isSerialThread());
    auto p = fs::path{payload};
    engine.loadSampleIntoSelectedPartAndGroup(p);
}
CLIENT_TO_SERIAL(AddSample, c2s_add_sample, std::string, addSample(payload, engine, cont);)

// sample, root, midi start end, vel start end
using addSampleWithRange_t = std::tuple<std::string, int, int, int, int, int>;
inline void addSampleWithRange(const addSampleWithRange_t &payload, engine::Engine &engine,
                               MessageController &cont)
{
    assert(cont.threadingChecker.isSerialThread());
    auto p = fs::path{std::get<0>(payload)};
    auto rk = std::get<1>(payload);
    auto kr = engine::KeyboardRange(std::get<2>(payload), std::get<3>(payload));
    auto vr = engine::VelocityRange(std::get<4>(payload), std::get<5>(payload));
    engine.loadSampleIntoSelectedPartAndGroup(p, rk, kr, vr);
}
CLIENT_TO_SERIAL(AddSampleWithRange, c2s_add_sample_with_range, addSampleWithRange_t,
                 addSampleWithRange(payload, engine, cont);)

inline void createGroupIn(int partNumber, engine::Engine &engine, MessageController &cont)
{
    if (partNumber < 0 || partNumber > numParts)
        partNumber = 0;
    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [p = partNumber](auto &e) { e.getPatch()->getPart(p)->addGroup(); },
        [p = partNumber](auto &engine) {
            serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(p),
                                      *(engine.getMessageController()));

            serializationSendToClient(s2c_send_selected_group_zone_mapping_summary,
                                      engine.getPatch()->getPart(p)->getZoneMappingSummary(),
                                      *(engine.getMessageController()));
            if (engine.getSelectionManager()->currentlySelectedZones().empty())
            {
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
    engine::Zone *zoneToFree{nullptr};
    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [s = a, &zoneToFree](auto &e) {
            auto zid = e.getPatch()->getPart(s.part)->getGroup(s.group)->getZone(s.zone)->id;
            auto z = e.getPatch()->getPart(s.part)->getGroup(s.group)->removeZone(zid);
            zoneToFree = z.release();
        },
        [t = a, &zoneToFree](auto &engine) {
            if (zoneToFree)
            {
                delete zoneToFree;
            }
            engine.getSampleManager()->purgeUnreferencedSamples();
            engine.getSelectionManager()->guaranteeConsistencyAfterDeletes(engine);
            serializationSendToClient(s2c_send_pgz_structure,
                                      engine.getPartGroupZoneStructure(t.part),
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
    auto zs = engine.getSelectionManager()->allSelectedZones;

    if (zs.empty())
        return;
    auto part = zs.begin()->part;

    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [sz = zs](auto &e) {
            // I know this allocates and deallocates on audio thread
            std::vector<std::tuple<int, int, ZoneID>> ids;
            for (auto s : sz)
                ids.push_back(
                    {s.part, s.group,
                     e.getPatch()->getPart(s.part)->getGroup(s.group)->getZone(s.zone)->id});
            for (auto [p, g, zid] : ids)
                e.getPatch()->getPart(p)->getGroup(g)->removeZone(zid);
        },
        [t = part](auto &engine) {
            engine.getSampleManager()->purgeUnreferencedSamples();
            engine.getSelectionManager()->guaranteeConsistencyAfterDeletes(engine);

            serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(t),
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
    engine::Group *groupToFree{nullptr};

    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [s = a, &groupToFree](auto &e) {
            if (e.getPatch()->getPart(s.part)->getGroups().size() <= 1)
            {
                groupToFree = nullptr;
            }
            else
            {
                auto gid = e.getPatch()->getPart(s.part)->getGroup(s.group)->id;
                groupToFree = e.getPatch()->getPart(s.part)->removeGroup(gid).release();
            }
        },
        [t = a, &groupToFree](auto &engine) {
            if (groupToFree)
            {
                delete groupToFree;
            }
            else
            {
                engine.getMessageController()->reportErrorToClient(
                    "Unable to Remove Group", "All parts must have one group; you tried to "
                                              "remove the last group in the part");
            }
            engine.getSampleManager()->purgeUnreferencedSamples();
            engine.getSelectionManager()->guaranteeConsistencyAfterDeletes(engine);

            serializationSendToClient(s2c_send_pgz_structure,
                                      engine.getPartGroupZoneStructure(t.part),
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
                auto gid = part->getGroup(0)->id;
                auto g = part->removeGroup(gid);
            }
        },
        [pt = p](auto &engine) {
            engine.getSampleManager()->purgeUnreferencedSamples();
            engine.getSelectionManager()->guaranteeConsistencyAfterDeletes(engine);

            serializationSendToClient(s2c_send_pgz_structure, engine.getPartGroupZoneStructure(pt),
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

    auto nad = engine.getPatch()->getPart(tgt.part)->getGroup(tgt.group)->getZones().size();

    cont.scheduleAudioThreadCallbackUnderStructureLock(
        [s = src, t = tgt](auto &e) {
            auto zid = e.getPatch()->getPart(s.part)->getGroup(s.group)->getZone(s.zone)->id;
            auto z = e.getPatch()->getPart(s.part)->getGroup(s.group)->removeZone(zid);
            if (z)
                e.getPatch()->getPart(t.part)->getGroup(t.group)->addZone(z);
        },
        [nad, t = tgt](auto &engine) {
            auto tc = t;
            tc.zone = nad;
            auto act = selection::SelectionManager::SelectActionContents(tc, true, true, true);
            engine.getSelectionManager()->selectAction(act);
            serializationSendToClient(s2c_send_pgz_structure,
                                      engine.getPartGroupZoneStructure(t.part),
                                      *(engine.getMessageController()));
            serializationSendToClient(s2c_send_selected_group_zone_mapping_summary,
                                      engine.getPatch()->getPart(t.part)->getZoneMappingSummary(),
                                      *(engine.getMessageController()));
        });
}
CLIENT_TO_SERIAL(MoveZoneFromTo, c2s_move_zone, zoneAddressFromTo_t,
                 moveZoneFromTo(payload, engine, cont));

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_STRUCTURE_MESSAGES_H
