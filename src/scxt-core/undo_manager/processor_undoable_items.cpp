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

#include "processor_undoable_items.h"

#include "engine/engine.h"
#include "engine/group.h"
#include "engine/zone.h"
#include "engine/part.h"
#include "messaging/messaging.h"
#include "messaging/client/client_serial.h"
#include "modulation/voice_matrix.h"
#include "modulation/group_matrix.h"

namespace scxt::undo
{

// --- ZoneProcessorTypeChangeItem ---

void ZoneProcessorTypeChangeItem::store(
    engine::Engine &e, int32_t which,
    const std::vector<selection::SelectionManager::ZoneAddress> &zones)
{
    forZone = true;
    whichProcessor = which;
    selectionList = zones;
    cachedStorage.clear();
    cachedStorage.reserve(zones.size());
    for (const auto &z : zones)
    {
        auto &zone = e.getPatch()->getPart(z.part)->getGroup(z.group)->getZone(z.zone);
        cachedStorage.push_back(zone->processorStorage[which]);
    }
}

void ZoneProcessorTypeChangeItem::restore(engine::Engine &e)
{
    auto which = whichProcessor;
    auto zs = selectionList;
    auto storage = std::move(cachedStorage);

    e.getMessageController()->scheduleAudioThreadCallback(
        [which, zs, storage](auto &eng) {
            for (size_t i = 0; i < zs.size() && i < storage.size(); ++i)
            {
                auto &zone =
                    eng.getPatch()->getPart(zs[i].part)->getGroup(zs[i].group)->getZone(zs[i].zone);
                zone->setProcessorType(which, storage[i].type);
                zone->processorStorage[which] = storage[i];
            }
        },
        [which](auto &eng) {
            auto lz = eng.getSelectionManager()->currentLeadZone(eng);
            if (lz.has_value())
            {
                auto &z = eng.getPatch()->getPart(lz->part)->getGroup(lz->group)->getZone(lz->zone);
                messaging::client::serializationSendToClient(
                    messaging::client::s2c_respond_single_processor_metadata_and_data,
                    messaging::client::ProcessorMetadataAndData::s2c_payload_t{
                        true, which, true, z->processorDescription[which],
                        z->processorStorage[which]},
                    *(eng.getMessageController()));
                messaging::client::serializationSendToClient(
                    messaging::client::s2c_update_zone_matrix_metadata,
                    voice::modulation::getVoiceMatrixMetadata(*z), *(eng.getMessageController()));
            }
        });
}

std::unique_ptr<UndoableItem> ZoneProcessorTypeChangeItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<ZoneProcessorTypeChangeItem>();
    redo->store(e, whichProcessor, selectionList);
    return redo;
}

// --- GroupProcessorTypeChangeItem ---

void GroupProcessorTypeChangeItem::store(
    engine::Engine &e, int32_t which,
    const std::vector<selection::SelectionManager::ZoneAddress> &groups)
{
    forZone = false;
    whichProcessor = which;
    selectionList = groups;
    cachedStorage.clear();
    cachedStorage.reserve(groups.size());
    for (const auto &g : groups)
    {
        auto &grp = e.getPatch()->getPart(g.part)->getGroup(g.group);
        cachedStorage.push_back(grp->processorStorage[which]);
    }
}

void GroupProcessorTypeChangeItem::restore(engine::Engine &e)
{
    auto which = whichProcessor;
    auto gs = selectionList;
    auto storage = std::move(cachedStorage);

    e.getMessageController()->scheduleAudioThreadCallback(
        [which, gs, storage](auto &eng) {
            for (size_t i = 0; i < gs.size() && i < storage.size(); ++i)
            {
                auto &grp = eng.getPatch()->getPart(gs[i].part)->getGroup(gs[i].group);
                grp->setProcessorType(which, storage[i].type);
                grp->processorStorage[which] = storage[i];
            }
        },
        [which](auto &eng) {
            auto lg = eng.getSelectionManager()->currentLeadGroup(eng);
            if (lg.has_value())
            {
                auto &g = eng.getPatch()->getPart(lg->part)->getGroup(lg->group);
                messaging::client::serializationSendToClient(
                    messaging::client::s2c_respond_single_processor_metadata_and_data,
                    messaging::client::ProcessorMetadataAndData::s2c_payload_t{
                        false, which, true, g->processorDescription[which],
                        g->processorStorage[which]},
                    *(eng.getMessageController()));
                messaging::client::serializationSendToClient(
                    messaging::client::s2c_update_group_matrix_metadata,
                    modulation::getGroupMatrixMetadata(*g), *(eng.getMessageController()));
            }
        });
}

std::unique_ptr<UndoableItem> GroupProcessorTypeChangeItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<GroupProcessorTypeChangeItem>();
    redo->store(e, whichProcessor, selectionList);
    return redo;
}

std::string ZoneProcessorTypeChangeItem::describe() const
{
    return "Update Zone Processor Type " + std::to_string(whichProcessor);
}

std::string GroupProcessorTypeChangeItem::describe() const
{
    return "Update Group Processor Type " + std::to_string(whichProcessor);
}

} // namespace scxt::undo
