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

#include "modulation_undoable_items.h"

#include "engine/engine.h"
#include "engine/group.h"
#include "engine/zone.h"
#include "engine/part.h"
#include "messaging/messaging.h"
#include "messaging/client/client_serial.h"

namespace scxt::undo
{

// --- ZoneModRowChangeItem ---

void ZoneModRowChangeItem::store(engine::Engine &e, int index,
                                 const std::vector<selection::SelectionManager::ZoneAddress> &zones)
{
    forZone = true;
    routeIndex = index;
    selectionList = zones;
    cachedRows.clear();
    cachedRows.reserve(zones.size());
    for (const auto &z : zones)
    {
        auto &zone = e.getPatch()->getPart(z.part)->getGroup(z.group)->getZone(z.zone);
        cachedRows.push_back(zone->routingTable.routes[index]);
    }
}

void ZoneModRowChangeItem::restore(engine::Engine &e)
{
    auto idx = routeIndex;
    auto zs = selectionList;
    auto rows = std::move(cachedRows);

    e.getMessageController()->scheduleAudioThreadCallback(
        [idx, zs, rows](auto &eng) {
            for (size_t i = 0; i < zs.size() && i < rows.size(); ++i)
            {
                auto &zone =
                    eng.getPatch()->getPart(zs[i].part)->getGroup(zs[i].group)->getZone(zs[i].zone);
                zone->routingTable.routes[idx] = rows[i];
                zone->onRoutingChanged();
            }
        },
        [](auto &eng) {
            auto lz = eng.getSelectionManager()->currentLeadZone(eng);
            if (lz.has_value())
            {
                eng.getSelectionManager()->sendDisplayDataForZonesBasedOnLead(lz->part, lz->group,
                                                                              lz->zone);
            }
        });
}

std::unique_ptr<UndoableItem> ZoneModRowChangeItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<ZoneModRowChangeItem>();
    redo->store(e, routeIndex, selectionList);
    return redo;
}

// --- GroupModRowChangeItem ---

void GroupModRowChangeItem::store(
    engine::Engine &e, int index,
    const std::vector<selection::SelectionManager::ZoneAddress> &groups)
{
    forZone = false;
    routeIndex = index;
    selectionList = groups;
    cachedRows.clear();
    cachedRows.reserve(groups.size());
    for (const auto &g : groups)
    {
        auto &grp = e.getPatch()->getPart(g.part)->getGroup(g.group);
        cachedRows.push_back(grp->routingTable.routes[index]);
    }
}

void GroupModRowChangeItem::restore(engine::Engine &e)
{
    auto idx = routeIndex;
    auto gs = selectionList;
    auto rows = std::move(cachedRows);

    e.getMessageController()->scheduleAudioThreadCallback(
        [idx, gs, rows](auto &eng) {
            for (size_t i = 0; i < gs.size() && i < rows.size(); ++i)
            {
                auto &grp = eng.getPatch()->getPart(gs[i].part)->getGroup(gs[i].group);
                grp->routingTable.routes[idx] = rows[i];
                grp->onRoutingChanged();
                grp->rePrepareAndBindGroupMatrix();
            }
        },
        [](auto &eng) {
            auto lg = eng.getSelectionManager()->currentLeadGroup(eng);
            if (lg.has_value())
                eng.getSelectionManager()->sendDisplayDataForGroupsBasedOnLead(lg->part, lg->group);
        });
}

std::unique_ptr<UndoableItem> GroupModRowChangeItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<GroupModRowChangeItem>();
    redo->store(e, routeIndex, selectionList);
    return redo;
}

std::string ZoneModRowChangeItem::describe() const
{
    return "Update Zone Mod Matrix Row " + std::to_string(routeIndex);
}

std::string GroupModRowChangeItem::describe() const
{
    return "Update Group Mod Matrix Row " + std::to_string(routeIndex);
}

} // namespace scxt::undo
