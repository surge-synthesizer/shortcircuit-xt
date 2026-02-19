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

#include "group_undoable_items.h"

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include <tao/json/contrib/traits.hpp>

#include "json/scxt_traits.h"
#include "json/engine_traits.h"

#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "messaging/messaging.h"
#include "messaging/audio/audio_messages.h"
#include "messaging/client/client_serial.h"

namespace scxt::undo
{

void GroupChangeItem::store(engine::Engine &e, int16_t part, int16_t group)
{
    partIndex = part;
    groupIndex = group;

    auto &pt = e.getPatch()->getPart(part);
    if (group >= (int16_t)pt->getGroups().size())
    {
        deleteGroupOnUndo = true;
        cachedGroupData.clear();
        return;
    }

    deleteGroupOnUndo = false;
    auto &g = pt->getGroup(group);
    auto jv = json::scxt_value(*g);
    cachedGroupData = tao::json::to_string(jv);
}

void GroupChangeItem::restore(engine::Engine &e)
{
    auto pi = partIndex;
    auto gi = groupIndex;
    auto del = deleteGroupOnUndo;
    auto data = std::move(cachedGroupData);
    e.getMessageController()->stopAudioThreadThenRunOnSerial(
        [pi, gi, del, data = std::move(data)](const auto &engine) {
            auto &e = const_cast<engine::Engine &>(engine);
            auto &pt = e.getPatch()->getPart(pi);

            if (del)
            {
                if (gi < (int16_t)pt->getGroups().size())
                {
                    auto &groupO = pt->getGroup(gi);
                    auto gid = groupO->id;
                    pt->removeGroup(gid);
                }
            }
            else
            {
                auto &g = pt->getGroup(gi);

                // Clear existing zones from the group
                g->clearZones();

                // Unstream the cached data back onto the group
                tao::json::events::transformer<tao::json::events::to_basic_value<json::scxt_traits>>
                    consumer;
                tao::json::events::from_string(consumer, data);
                auto jv = std::move(consumer.value);
                jv.to(*g);

                g->setupOnUnstream(e);
            }

            e.sendFullRefreshToClient();
            e.getMessageController()->restartAudioThreadFromSerial();
        });
}

std::unique_ptr<UndoableItem> GroupChangeItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<GroupChangeItem>();
    redo->store(e, partIndex, groupIndex);
    return redo;
}

void GroupRenameItem::store(engine::Engine &e, int16_t part, int16_t group)
{
    partIndex = part;
    groupIndex = group;
    oldName = e.getPatch()->getPart(part)->getGroup(group)->name;
}

void GroupRenameItem::restore(engine::Engine &e)
{
    e.getPatch()->getPart(partIndex)->getGroup(groupIndex)->name = oldName;
    messaging::client::serializationSendToClient(messaging::client::s2c_send_pgz_structure,
                                                 e.getPartGroupZoneStructure(),
                                                 *(e.getMessageController()));
}

std::unique_ptr<UndoableItem> GroupRenameItem::makeRedo(engine::Engine &e)
{
    auto redo = std::make_unique<GroupRenameItem>();
    redo->store(e, partIndex, groupIndex);
    return redo;
}

std::string GroupChangeItem::describe() const
{
    return "Group Change [part=" + std::to_string(partIndex) +
           ", group=" + std::to_string(groupIndex) + (deleteGroupOnUndo ? ", delete-on-undo" : "") +
           "]";
}

std::string GroupRenameItem::describe() const
{
    return "Rename Group [part=" + std::to_string(partIndex) +
           ", group=" + std::to_string(groupIndex) + ", name=" + oldName + "]";
}

} // namespace scxt::undo
