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

#include "structure_undoable_items.h"

#include <algorithm>

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "json/scxt_traits.h"
#include "json/engine_traits.h"
#include "json/stream.h"

#include "engine/engine.h"
#include "engine/zone.h"
#include "engine/group.h"
#include "engine/part.h"
#include "messaging/messaging.h"
#include "messaging/audio/audio_messages.h"
#include "messaging/client/client_serial.h"
#include "messaging/client/detail/client_serial_impl.h"
#include "messaging/client/structure_messages.h"

namespace scxt::undo
{

namespace
{
using ZoneAddress = selection::SelectionManager::ZoneAddress;

void sendStructureAndSummary(const engine::Engine &engine, int16_t part)
{
    messaging::client::serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                                 engine.getPartGroupZoneStructure(),
                                                 *(engine.getMessageController()));
    messaging::client::serializationSendToClient(
        messaging::client::s2c_send_selected_group_zone_mapping_summary,
        engine.getPatch()->getPart(part)->getZoneMappingSummary(),
        *(engine.getMessageController()));
}
} // namespace

// --- ZonesDeleteOnUndoItem ---

void ZonesDeleteOnUndoItem::store(engine::Engine &e, const std::vector<ZoneAddress> &addrs)
{
    addresses = addrs;
    std::sort(addresses.begin(), addresses.end());
}

void ZonesDeleteOnUndoItem::restore(engine::Engine &e)
{
    auto addrs = addresses;
    e.getMessageController()->scheduleAudioThreadCallbackUnderStructureLock(
        [addrs](auto &eng) {
            // descending so earlier indices stay valid
            for (auto it = addrs.rbegin(); it != addrs.rend(); ++it)
            {
                auto &grp = eng.getPatch()->getPart(it->part)->getGroup(it->group);
                if (it->zone >= (int)grp->getZones().size())
                    continue;
                auto &zoneO = grp->getZone(it->zone);
                eng.terminateVoicesForZone(*zoneO);
                auto z = grp->removeZone(zoneO->id);
                eng.getMessageController()->sendItemForDeletion(
                    z.release(), messaging::audio::AudioToSerialization::ToBeDeleted::engine_Zone);
            }
        },
        [t = addresses.front()](auto &engine) {
            // no sample purge: a redo wants the samples resident
            engine.getSelectionManager()->guaranteeConsistencyAfterDeletes(engine, true, t);
            sendStructureAndSummary(engine, t.part);
        });
}

std::unique_ptr<UndoableItem> ZonesDeleteOnUndoItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<ZonesRestoreItem>();
    redo->store(e, addresses);
    return redo;
}

std::string ZonesDeleteOnUndoItem::describe() const
{
    return "Add Zone [" + std::to_string(addresses.size()) + " zones]";
}

void pushZoneAddUndo(engine::Engine &e, int16_t part, int32_t group)
{
    auto &pt = e.getPatch()->getPart(part);
    int32_t zi =
        (group < (int)pt->getGroups().size()) ? (int32_t)pt->getGroup(group)->getZones().size() : 0;
    auto item = std::make_unique<ZonesDeleteOnUndoItem>();
    item->store(e, {{part, group, zi}});
    e.undoManager.storeUndoStep(e, std::move(item));
}

// --- ZonesRestoreItem ---

void ZonesRestoreItem::store(engine::Engine &e, const std::vector<ZoneAddress> &addrs)
{
    auto sorted = addrs;
    std::sort(sorted.begin(), sorted.end());
    zones.clear();
    zones.reserve(sorted.size());
    for (const auto &a : sorted)
    {
        auto &z = e.getPatch()->getPart(a.part)->getGroup(a.group)->getZone(a.zone);
        auto jv = json::scxt_value(*z);
        zones.emplace_back(a, tao::json::to_string(jv));
    }
}

void ZonesRestoreItem::restore(engine::Engine &e)
{
    // rebuild serial side - the json unstream attaches samples which is
    // serial-only work - then insert under the structure lock
    std::vector<std::pair<ZoneAddress, engine::Zone *>> rebuilt;
    rebuilt.reserve(zones.size());
    for (const auto &[a, data] : zones)
    {
        auto zptr = std::make_unique<engine::Zone>();
        zptr->engine = &e;

        tao::json::events::transformer<tao::json::events::to_basic_value<json::scxt_traits>>
            consumer;
        tao::json::events::from_string(consumer, data);
        auto jv = std::move(consumer.value);
        jv.to(*zptr);

        zptr->setupOnUnstream(e);
        rebuilt.emplace_back(a, zptr.release());
    }

    e.getMessageController()->scheduleAudioThreadCallbackUnderStructureLock(
        [rebuilt](auto &eng) {
            // ascending address order so each insert index is valid
            for (const auto &[a, zraw] : rebuilt)
            {
                std::unique_ptr<engine::Zone> zptr;
                zptr.reset(zraw);
                eng.getPatch()->getPart(a.part)->guaranteeGroupCount(a.group + 1);
                eng.getPatch()->getPart(a.part)->getGroup(a.group)->insertZone(zptr, a.zone);
            }
        },
        [addrs = zones](auto &engine) {
            // select the resurrected set with the first as lead
            bool first{true};
            for (const auto &[a, data] : addrs)
            {
                engine.getSelectionManager()->applySelectActions(
                    selection::SelectionManager::SelectActionContents(a, true, first, first));
                first = false;
            }
            sendStructureAndSummary(engine, addrs.front().first.part);
        });
}

std::unique_ptr<UndoableItem> ZonesRestoreItem::makeRedo(engine::Engine &e)
{
    std::vector<ZoneAddress> addrs;
    addrs.reserve(zones.size());
    for (const auto &[a, d] : zones)
        addrs.push_back(a);
    auto redo = std::make_unique<ZonesDeleteOnUndoItem>();
    redo->store(e, addrs);
    return redo;
}

std::string ZonesRestoreItem::describe() const
{
    return "Delete Zone [" + std::to_string(zones.size()) + " zones]";
}

// --- ZoneOrderRestoreItem ---

void ZoneOrderRestoreItem::store(engine::Engine &e, int16_t pt, const std::vector<int32_t> &groups)
{
    part = pt;
    groupOrderings.clear();
    auto &partO = e.getPatch()->getPart(pt);
    for (auto gi : groups)
    {
        if (gi < 0 || gi >= (int32_t)partO->getGroups().size())
            continue;
        std::vector<ZoneID> ids;
        for (const auto &z : *(partO->getGroup(gi)))
            ids.push_back(z->id);
        groupOrderings.emplace_back(gi, ids);
    }
}

void ZoneOrderRestoreItem::restore(engine::Engine &e)
{
    auto pt = part;
    auto orderings = groupOrderings;
    auto createFirst = createGroupsFirst;
    auto deleteAtEnd = deleteGroupsAtEnd;

    e.getMessageController()->scheduleAudioThreadCallbackUnderStructureLock(
        [pt, orderings, createFirst, deleteAtEnd](auto &eng) {
            auto &partO = eng.getPatch()->getPart(pt);

            for (auto gi : createFirst)
            {
                partO->guaranteeGroupCount(gi + 1);
            }

            // pull every involved zone out into a pool keyed by id
            std::vector<std::pair<ZoneID, std::unique_ptr<engine::Zone>>> pool;
            for (const auto &[gi, ids] : orderings)
            {
                for (const auto &zid : ids)
                {
                    for (auto &g : *partO)
                    {
                        if (g->getZoneIndex(zid) >= 0)
                        {
                            eng.terminateVoicesForZone(*(g->getZone(zid)));
                            pool.emplace_back(zid, g->removeZone(zid));
                            break;
                        }
                    }
                }
            }

            // refill in the stored order
            for (const auto &[gi, ids] : orderings)
            {
                auto &g = partO->getGroup(gi);
                for (const auto &zid : ids)
                {
                    auto it = std::find_if(pool.begin(), pool.end(),
                                           [&zid](const auto &p) { return p.first == zid; });
                    if (it != pool.end() && it->second)
                        g->addZone(std::move(it->second));
                }
            }

            for (auto it = deleteAtEnd.rbegin(); it != deleteAtEnd.rend(); ++it)
            {
                if (*it < (int32_t)partO->getGroups().size())
                {
                    auto &groupO = partO->getGroup(*it);
                    eng.terminateVoicesForGroup(*groupO);
                    auto groupToFree = partO->removeGroup(groupO->id).release();
                    eng.getMessageController()->sendItemForDeletion(
                        groupToFree,
                        messaging::audio::AudioToSerialization::ToBeDeleted::engine_Group);
                }
            }
        },
        [pt](auto &engine) {
            engine.getSelectionManager()->guaranteeConsistencyAfterDeletes(engine, true,
                                                                           {pt, -1, -1});
            sendStructureAndSummary(engine, pt);
        });
}

std::unique_ptr<UndoableItem> ZoneOrderRestoreItem::makeRedo(engine::Engine &e)
{
    // snapshot the groups as they are now; groups this item creates don't
    // exist yet so the redo deletes them, and vice versa
    std::vector<int32_t> nowGroups;
    for (const auto &[gi, ids] : groupOrderings)
    {
        if (std::find(createGroupsFirst.begin(), createGroupsFirst.end(), gi) ==
            createGroupsFirst.end())
            nowGroups.push_back(gi);
    }
    for (auto gi : deleteGroupsAtEnd)
        nowGroups.push_back(gi);

    auto redo = std::make_unique<ZoneOrderRestoreItem>();
    redo->store(e, part, nowGroups);
    redo->createGroupsFirst = deleteGroupsAtEnd;
    redo->deleteGroupsAtEnd = createGroupsFirst;
    return redo;
}

std::string ZoneOrderRestoreItem::describe() const
{
    return "Move Zones [part=" + std::to_string(part) + ", " +
           std::to_string(groupOrderings.size()) + " groups]";
}

namespace
{
std::unique_ptr<engine::Group> rebuildGroupFromJSON(engine::Engine &e, int16_t part,
                                                    const std::string &data)
{
    auto gptr = std::make_unique<engine::Group>(e.rng);
    gptr->parentPart = e.getPatch()->getPart(part).get();
    gptr->setSampleRate(e.getPatch()->getPart(part)->getSampleRate());

    tao::json::events::transformer<tao::json::events::to_basic_value<json::scxt_traits>> consumer;
    tao::json::events::from_string(consumer, data);
    auto jv = std::move(consumer.value);
    jv.to(*gptr);

    gptr->setupOnUnstream(e);
    return gptr;
}
} // namespace

// --- GroupsDeleteOnUndoItem ---

void GroupsDeleteOnUndoItem::store(engine::Engine &e,
                                   const std::vector<std::pair<int16_t, int32_t>> &addrs)
{
    addresses = addrs;
    std::sort(addresses.begin(), addresses.end());
}

void GroupsDeleteOnUndoItem::restore(engine::Engine &e)
{
    auto addrs = addresses;
    e.getMessageController()->scheduleAudioThreadCallbackUnderStructureLock(
        [addrs](auto &eng) {
            for (auto it = addrs.rbegin(); it != addrs.rend(); ++it)
            {
                auto &partO = eng.getPatch()->getPart(it->first);
                if (it->second >= (int32_t)partO->getGroups().size())
                    continue;
                auto &groupO = partO->getGroup(it->second);
                eng.terminateVoicesForGroup(*groupO);
                auto groupToFree = partO->removeGroup(groupO->id).release();
                eng.getMessageController()->sendItemForDeletion(
                    groupToFree, messaging::audio::AudioToSerialization::ToBeDeleted::engine_Group);
            }
        },
        [t = addresses.front()](auto &engine) {
            engine.getSelectionManager()->guaranteeConsistencyAfterDeletes(engine, false,
                                                                           {t.first, t.second, -1});
            sendStructureAndSummary(engine, t.first);
        });
}

std::unique_ptr<UndoableItem> GroupsDeleteOnUndoItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<GroupsRestoreItem>();
    redo->store(e, addresses);
    return redo;
}

std::string GroupsDeleteOnUndoItem::describe() const
{
    return "Add Group [" + std::to_string(addresses.size()) + " groups]";
}

// --- GroupsRestoreItem ---

void GroupsRestoreItem::store(engine::Engine &e,
                              const std::vector<std::pair<int16_t, int32_t>> &addrs)
{
    auto sorted = addrs;
    std::sort(sorted.begin(), sorted.end());
    groups.clear();
    groups.reserve(sorted.size());
    for (const auto &[pt, gi] : sorted)
    {
        auto &g = e.getPatch()->getPart(pt)->getGroup(gi);
        auto jv = json::scxt_value(*g);
        groups.push_back({pt, gi, tao::json::to_string(jv)});
    }
}

void GroupsRestoreItem::restore(engine::Engine &e)
{
    // group unstream re-attaches samples which is serial-only work, so this
    // matches the GroupChangeItem resurrect path and stops the audio thread
    auto entries = groups;
    e.getMessageController()->stopAudioThreadThenRunOnSerial([entries](const auto &engine) {
        auto &e = const_cast<engine::Engine &>(engine);
        for (const auto &ent : entries)
        {
            auto gptr = rebuildGroupFromJSON(e, ent.part, ent.groupJSON);
            e.getPatch()->getPart(ent.part)->insertGroup(gptr, ent.groupIndex);
        }
        auto lead = entries.front();
        e.getSelectionManager()->applySelectActions(
            {lead.part, lead.groupIndex, -1, true, true, true});
        e.sendFullRefreshToClient();
        e.getMessageController()->restartAudioThreadFromSerial();
    });
}

std::unique_ptr<UndoableItem> GroupsRestoreItem::makeRedo(engine::Engine &e)
{
    std::vector<std::pair<int16_t, int32_t>> addrs;
    addrs.reserve(groups.size());
    for (const auto &ent : groups)
        addrs.emplace_back(ent.part, ent.groupIndex);
    auto redo = std::make_unique<GroupsDeleteOnUndoItem>();
    redo->store(e, addrs);
    return redo;
}

std::string GroupsRestoreItem::describe() const
{
    return "Delete Group [" + std::to_string(groups.size()) + " groups]";
}

// --- GroupOrderRestoreItem ---

void GroupOrderRestoreItem::store(engine::Engine &e, int16_t pt)
{
    part = pt;
    order.clear();
    for (const auto &g : *(e.getPatch()->getPart(pt)))
        order.push_back(g->id);
}

void GroupOrderRestoreItem::restore(engine::Engine &e)
{
    auto pt = part;
    auto ord = order;
    e.getMessageController()->scheduleAudioThreadCallbackUnderStructureLock(
        [pt, ord](auto &eng) {
            auto &partO = eng.getPatch()->getPart(pt);
            eng.voiceManager.allSoundsOff();
            // selection-sort into the stored id order
            for (size_t target = 0; target < ord.size(); ++target)
            {
                auto cur = partO->getGroupIndex(ord[target]);
                if (cur >= 0 && cur != (int)target)
                    partO->swapGroups(cur, target);
            }
        },
        [pt](auto &engine) { sendStructureAndSummary(engine, pt); });
}

std::unique_ptr<UndoableItem> GroupOrderRestoreItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<GroupOrderRestoreItem>();
    redo->store(e, part);
    return redo;
}

std::string GroupOrderRestoreItem::describe() const
{
    return "Move Group [part=" + std::to_string(part) + "]";
}

// --- PartsStateItem ---

void PartsStateItem::store(engine::Engine &e, const std::vector<int16_t> &whichParts)
{
    parts.clear();
    for (auto pt : whichParts)
    {
        Entry ent;
        ent.part = pt;
        auto &partO = e.getPatch()->getPart(pt);
        ent.active = partO->configuration.active;
        for (const auto &g : *partO)
        {
            auto jv = json::scxt_value(*g);
            ent.groupsJSON.push_back(tao::json::to_string(jv));
        }
        parts.push_back(ent);
    }
}

void PartsStateItem::restore(engine::Engine &e)
{
    auto entries = parts;
    e.getMessageController()->stopAudioThreadThenRunOnSerial([entries](const auto &engine) {
        auto &e = const_cast<engine::Engine &>(engine);
        for (const auto &ent : entries)
        {
            auto &partO = e.getPatch()->getPart(ent.part);
            partO->clearGroups();
            for (const auto &gj : ent.groupsJSON)
            {
                auto gptr = rebuildGroupFromJSON(e, ent.part, gj);
                partO->addGroup(gptr);
            }
            partO->configuration.active = ent.active;
        }
        e.onPartConfigurationUpdated();
        e.sendFullRefreshToClient();
        e.getMessageController()->restartAudioThreadFromSerial();
    });
}

std::unique_ptr<UndoableItem> PartsStateItem::makeRedo(engine::Engine &e)
{
    std::vector<int16_t> whichParts;
    whichParts.reserve(parts.size());
    for (const auto &ent : parts)
        whichParts.push_back(ent.part);
    auto redo = std::make_unique<PartsStateItem>();
    redo->store(e, whichParts);
    return redo;
}

std::string PartsStateItem::describe() const
{
    return "Part State [" + std::to_string(parts.size()) + " parts]";
}

// --- PartStreamRestoreItem ---

void PartStreamRestoreItem::store(engine::Engine &e, int16_t pt, const std::string &lbl)
{
    part = pt;
    label = lbl;
    e.prepareToStream();
    auto sg = engine::Engine::StreamGuard(engine::Engine::FOR_PART);
    auto jv = json::scxt_value(*(e.getPatch()->getPart(pt)));
    partJSON = tao::json::to_string(jv);
}

void PartStreamRestoreItem::restore(engine::Engine &e)
{
    auto pt = part;
    auto payload = partJSON;
    e.getMessageController()->stopAudioThreadThenRunOnSerial([pt, payload](const auto &engine) {
        auto &e = const_cast<engine::Engine &>(engine);
        try
        {
            e.immediatelyTerminateAllVoices();
            // clears the groups then unstreams and sends a full refresh
            json::unstreamPartState(e, pt, payload);
            e.getSelectionManager()->guaranteeConsistencyAfterDeletes(e, true, {pt, -1, -1});
        }
        catch (std::exception &err)
        {
            RAISE_ERROR_ENGINE(e, "Undo Error",
                               std::string("Unable to restore part state ") + err.what());
        }
        e.getMessageController()->restartAudioThreadFromSerial();
    });
}

std::unique_ptr<UndoableItem> PartStreamRestoreItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<PartStreamRestoreItem>();
    redo->store(e, part, label);
    return redo;
}

std::string PartStreamRestoreItem::describe() const
{
    return label + " [part=" + std::to_string(part) + "]";
}

void pushPartStreamUndo(engine::Engine &e, int16_t part, const std::string &label, UndoGesture g)
{
    auto tag = "Part Stream/" + std::to_string(part);
    if (g == UndoGesture::Discrete && e.undoManager.gestureCovers(tag))
    {
        // skip the snapshot work, but notify the host of the state change

        messaging::audio::SerializationToAudio s2am;
        s2am.id = messaging::audio::s2a_state_mark_dirty;
        e.getMessageController()->sendSerializationToAudio(s2am);

        return;
    }

    undo::pushUndoTagged<PartStreamRestoreItem>(e, tag, g, part, label);
}

// --- EngineStateRestoreItem ---

void EngineStateRestoreItem::store(engine::Engine &e)
{
    e.prepareToStream();
    engineJSON = json::streamEngineState(e);
}

void EngineStateRestoreItem::restore(engine::Engine &e)
{
    // mirrors the unstream-engine-state handler dispatch
    auto payload = engineJSON;
    auto &cont = *e.getMessageController();
    if (cont.isAudioRunning)
    {
        cont.stopAudioThreadThenRunOnSerial([payload](const auto &engine) {
            auto &e = const_cast<engine::Engine &>(engine);
            try
            {
                e.immediatelyTerminateAllVoices();
                json::unstreamEngineState(e, payload);
                e.getMessageController()->restartAudioThreadFromSerial();
            }
            catch (std::exception &err)
            {
                RAISE_ERROR_ENGINE(e, "Undo Error",
                                   std::string("Unable to restore engine state ") + err.what());
            }
        });
    }
    else
    {
        try
        {
            e.stopEngineRequests++;
            e.immediatelyTerminateAllVoices();
            json::unstreamEngineState(e, payload);
            e.stopEngineRequests--;
        }
        catch (std::exception &err)
        {
            RAISE_ERROR_ENGINE(e, "Undo Error",
                               std::string("Unable to restore engine state ") + err.what());
        }
    }
}

std::unique_ptr<UndoableItem> EngineStateRestoreItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<EngineStateRestoreItem>();
    redo->store(e);
    return redo;
}

std::string EngineStateRestoreItem::describe() const { return "Load Multi"; }

} // namespace scxt::undo
