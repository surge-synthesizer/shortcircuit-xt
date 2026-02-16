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

#include "zone_undoable_items.h"

#include "engine/engine.h"
#include "engine/zone.h"
#include "engine/group.h"
#include "engine/part.h"
#include "messaging/messaging.h"
#include "messaging/client/client_serial.h"

namespace scxt::undo
{

void ZoneMappingDataUndoableItem::store(engine::Engine &e)
{
    auto sz = e.getSelectionManager()->currentlySelectedZones();
    selectionList.assign(sz.begin(), sz.end());
    cachedMappingData.clear();
    cachedMappingData.reserve(selectionList.size());
    for (const auto &z : selectionList)
    {
        auto &zone = e.getPatch()->getPart(z.part)->getGroup(z.group)->getZone(z.zone);
        cachedMappingData.push_back(zone->mapping);
    }
}

void ZoneMappingDataUndoableItem::restore(engine::Engine &e)
{
    auto zs = selectionList;
    auto mappings = std::move(cachedMappingData);

    e.getMessageController()->scheduleAudioThreadCallback(
        [zs, mappings](auto &eng) {
            for (size_t i = 0; i < zs.size() && i < mappings.size(); ++i)
            {
                auto &zone =
                    eng.getPatch()->getPart(zs[i].part)->getGroup(zs[i].group)->getZone(zs[i].zone);
                zone->mapping = mappings[i];
            }
        },
        [](auto &eng) {
            auto lz = eng.getSelectionManager()->currentLeadZone(eng);
            if (lz.has_value())
            {
                eng.getSelectionManager()->sendDisplayDataForZonesBasedOnLead(lz->part, lz->group,
                                                                              lz->zone);
            }
            auto pt = eng.getSelectionManager()->currentlySelectedPart(eng);
            messaging::client::serializationSendToClient(
                messaging::client::s2c_send_selected_group_zone_mapping_summary,
                eng.getPatch()->getPart(pt)->getZoneMappingSummary(),
                *(eng.getMessageController()));
        });
}

std::unique_ptr<UndoableItem> ZoneMappingDataUndoableItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<ZoneMappingDataUndoableItem>();
    redo->store(e);
    return redo;
}

std::string ZoneMappingDataUndoableItem::describe() const
{
    return "Zone Mapping Data [" + std::to_string(selectionList.size()) + " zones]";
}

} // namespace scxt::undo
