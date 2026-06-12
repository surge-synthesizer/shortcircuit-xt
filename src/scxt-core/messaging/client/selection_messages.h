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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_SELECTION_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_SELECTION_MESSAGES_H

#include "client_macros.h"
#include "messaging/messaging.h"
#include "selection/selection_manager.h"
#include "undo_manager/payload_undoable_items.h"
#include "undo_manager/structure_undoable_items.h"

namespace scxt::messaging::client
{
inline void
doApplySelectActions(const std::vector<selection::SelectionManager::SelectActionContents> &za,
                     const engine::Engine &engine)
{
    engine.getSelectionManager()->applySelectActions(za);
}
CLIENT_TO_SERIAL(ApplySelectActions, c2s_apply_select_actions,
                 std::vector<selection::SelectionManager::SelectActionContents>,
                 doApplySelectActions(payload, engine));

CLIENT_TO_SERIAL(SelectPart, c2s_select_part, int16_t,
                 engine.getSelectionManager()->selectPart(payload));

// Lead Zone, Zone Selection, Gropu Selection
typedef std::tuple<std::optional<selection::SelectionManager::ZoneAddress>,
                   selection::SelectionManager::selectedZones_t,
                   std::optional<selection::SelectionManager::ZoneAddress>,
                   selection::SelectionManager::selectedZones_t,
                   selection::SelectionManager::selectedZones_t>
    selectedStateMessage_t;
SERIAL_TO_CLIENT(SetSelectionState, s2c_send_selection_state, selectedStateMessage_t,
                 onSelectionState);

SERIAL_TO_CLIENT(SetOtherTabSelection, s2c_send_othertab_selection,
                 scxt::selection::SelectionManager::otherTabSelection_t, onOtherTabSelection);

using updateOther_t = std::pair<std::string, std::string>;
inline void doUpdateOtherTabSelection(const updateOther_t &pl, const engine::Engine &engine)
{
    engine.getSelectionManager()->otherTabSelection[pl.first] = pl.second;
}
CLIENT_TO_SERIAL(UpdateOtherTabSelection, c2s_set_othertab_selection, updateOther_t,
                 doUpdateOtherTabSelection(payload, engine));

/*
 * Identifies which editable subtree a continuous gesture is about to touch
 * so begin-edit can snapshot it for undo. Undo is per event, not per change:
 * the entry pushed at gesture begin is the only one for the whole drag.
 */
enum struct EditSubtree : int32_t
{
    none = 0,     // gesture has no undo subtree (yet); begin is a no-op
    zone_mapping, // forZone implicit
    output_info,  // zone or group by forZone
    eg,           // index = which envelope
    mod_storage,  // index = which modulator
    misc_mod,
    audio_mod,
    processor,   // index = slot
    mod_row,     // index = row
    bus_effect,  // index = undo::mixerUndoIndex(bus, part, slot)
    bus_send,    // index = bus
    part_stream, // index = part; snapshots the whole part. The replace-part
                 // drop sends this before clear + import so the sequence is
                 // one undo entry; the gesture closes on the next unrelated
                 // push (no end-edit: the import work is deferred)
};

// subtree, forZone, index
using editGestureBegin_t = std::tuple<int32_t, bool, int32_t>;
inline void doBeginEdit(const editGestureBegin_t &payload, engine::Engine &engine,
                        messaging::MessageController &cont)
{
    auto [st, forZone, idx] = payload;
    constexpr auto B = undo::UndoGesture::Begin;
    switch ((EditSubtree)st)
    {
    case EditSubtree::none:
        break;
    case EditSubtree::zone_mapping:
        undo::pushPayloadUndo<undo::ZoneMappingSpec>(engine, -1, B);
        break;
    case EditSubtree::output_info:
        undo::pushZoneOrGroupPayloadUndo<undo::ZoneOutputInfoSpec, undo::GroupOutputInfoSpec>(
            engine, forZone, -1, B);
        break;
    case EditSubtree::eg:
        undo::pushZoneOrGroupPayloadUndo<undo::ZoneEgSpec, undo::GroupEgSpec>(engine, forZone, idx,
                                                                              B);
        break;
    case EditSubtree::mod_storage:
        undo::pushZoneOrGroupPayloadUndo<undo::ZoneModStorageSpec, undo::GroupModStorageSpec>(
            engine, forZone, idx, B);
        break;
    case EditSubtree::misc_mod:
        undo::pushZoneOrGroupPayloadUndo<undo::ZoneMiscModSpec, undo::GroupMiscModSpec>(
            engine, forZone, -1, B);
        break;
    case EditSubtree::audio_mod:
        undo::pushZoneOrGroupPayloadUndo<undo::ZoneAudioModSpec, undo::GroupAudioModSpec>(
            engine, forZone, -1, B);
        break;
    case EditSubtree::processor:
        undo::pushZoneOrGroupPayloadUndo<undo::ZoneProcessorSpec, undo::GroupProcessorSpec>(
            engine, forZone, idx, B);
        break;
    case EditSubtree::mod_row:
        undo::pushZoneOrGroupPayloadUndo<undo::ZoneModRowSpec, undo::GroupModRowSpec>(
            engine, forZone, idx, B);
        break;
    case EditSubtree::bus_effect:
    {
        auto slot = idx & 0xff;
        auto bus = ((idx >> 8) & 0xff) - 1;
        auto pt = ((idx >> 16) & 0xff) - 1;
        undo::pushPayloadUndoFor<undo::BusEffectSpec>(engine, {{pt, bus, slot}}, idx, B);
        break;
    }
    case EditSubtree::bus_send:
        undo::pushPayloadUndoFor<undo::BusSendSpec>(engine, {{-1, idx, -1}},
                                                    undo::mixerUndoIndex(idx, -1, -1), B);
        break;
    case EditSubtree::part_stream:
        undo::pushPartStreamUndo(engine, idx, "Replace Part", B);
        break;
    }
}
CLIENT_TO_SERIAL(BeginEdit, c2s_begin_edit, editGestureBegin_t, doBeginEdit(payload, engine, cont));

using editGestureFor_t = bool;
inline void doEndEdit(const editGestureFor_t &payload, engine::Engine &engine,
                      messaging::MessageController &cont)
{
    engine.undoManager.closeGesture();

    // this is way too much to send on each end edit. It's just
    // serial to ui so we hve time but.... see the #775 discussion
    auto lz = engine.getSelectionManager()->currentLeadZone(engine);
    if (lz.has_value())
    {
        engine.getSelectionManager()->configureAndSendZoneOrGroupModMatrixMetadata(
            lz->part, lz->group, lz->zone);
    }
    auto lg = engine.getSelectionManager()->currentLeadGroup(engine);
    if (lg.has_value())
    {
        engine.getSelectionManager()->configureAndSendZoneOrGroupModMatrixMetadata(lg->part,
                                                                                   lg->group, -1);
    }
}
CLIENT_TO_SERIAL(EndEdit, c2s_end_edit, editGestureFor_t, doEndEdit(payload, engine, cont));

// Group/zone tree fold state. Per-part set of collapsed group indices.
// The result is broadcast back to the client via the regular structure
// broadcast — the FOLDED bit is stamped onto group-row features.
using setGroupCollapsedPayload_t = std::tuple<int32_t, int32_t, bool>; // part, group, collapsed
CLIENT_TO_SERIAL(SetGroupCollapsed, c2s_set_group_collapsed, setGroupCollapsedPayload_t,
                 engine.getSelectionManager()->setGroupCollapsed(std::get<0>(payload),
                                                                 std::get<1>(payload),
                                                                 std::get<2>(payload)));

using setAllGroupsCollapsedPayload_t = std::pair<int32_t, bool>; // part, collapsed
CLIENT_TO_SERIAL(SetAllGroupsCollapsed, c2s_set_all_groups_collapsed,
                 setAllGroupsCollapsedPayload_t,
                 engine.getSelectionManager()->setAllGroupsCollapsed(payload.first,
                                                                     payload.second));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_SELECTION_MESSAGES_H
