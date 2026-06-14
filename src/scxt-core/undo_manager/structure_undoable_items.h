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

#ifndef SCXT_SRC_SCXT_CORE_UNDO_MANAGER_STRUCTURE_UNDOABLE_ITEMS_H
#define SCXT_SRC_SCXT_CORE_UNDO_MANAGER_STRUCTURE_UNDOABLE_ITEMS_H

/*
 * Structural undo: zone and group lifecycle (add / delete / move). These
 * come in symmetric pairs - an item which deletes addresses on restore
 * makes a redo which resurrects them from JSON, and vice versa - plus an
 * ordering snapshot for moves. The multi-element forms serve the single
 * element cases with a one-entry list.
 */

#include <string>
#include <vector>

#include "undoable_items.h"
#include "undo.h"
#include "utils.h"

namespace scxt::undo
{

struct ZonesRestoreItem;

// undo of an add: delete the zones at these addresses. Redo resurrects them.
struct ZonesDeleteOnUndoItem : public UndoableItem
{
    std::vector<selection::SelectionManager::ZoneAddress> addresses;

    void store(engine::Engine &e,
               const std::vector<selection::SelectionManager::ZoneAddress> &addrs);
    void restore(engine::Engine &e) override;
    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override;
    std::string describe() const override;
};

// Push the undo step for adding a single zone to (part, group): undoing
// deletes the zone at the index the add will land on (the end of the group).
void pushZoneAddUndo(engine::Engine &e, int16_t part, int32_t group);

// undo of a delete: resurrect zones from JSON at their original addresses.
// Redo deletes them again.
struct ZonesRestoreItem : public UndoableItem
{
    std::vector<std::pair<selection::SelectionManager::ZoneAddress, std::string>> zones;

    void store(engine::Engine &e,
               const std::vector<selection::SelectionManager::ZoneAddress> &addrs);
    void restore(engine::Engine &e) override;
    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override;
    std::string describe() const override;
};

// undo of a move: restore the zone-id ordering of the affected groups in
// one part. Zones are looked up by id so their contents ride along
// unchanged. Groups created by the move are deleted on restore; groups the
// restore needs which don't exist yet (the redo of such a move) are created
// first.
struct ZoneOrderRestoreItem : public UndoableItem
{
    int16_t part{-1};
    std::vector<std::pair<int32_t, std::vector<ZoneID>>> groupOrderings;
    std::vector<int32_t> createGroupsFirst;
    std::vector<int32_t> deleteGroupsAtEnd;

    void store(engine::Engine &e, int16_t pt, const std::vector<int32_t> &groups);
    void restore(engine::Engine &e) override;
    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override;
    std::string describe() const override;
};

struct GroupsRestoreItem;

// undo of a group add: delete the groups at (part, index). Redo resurrects.
struct GroupsDeleteOnUndoItem : public UndoableItem
{
    std::vector<std::pair<int16_t, int32_t>> addresses; // part, group index

    void store(engine::Engine &e, const std::vector<std::pair<int16_t, int32_t>> &addrs);
    void restore(engine::Engine &e) override;
    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override;
    std::string describe() const override;
};

// undo of a group delete: resurrect groups from JSON at their original
// indices. Redo deletes them again.
struct GroupsRestoreItem : public UndoableItem
{
    struct Entry
    {
        int16_t part{-1};
        int32_t groupIndex{-1};
        std::string groupJSON;
    };
    std::vector<Entry> groups;

    void store(engine::Engine &e, const std::vector<std::pair<int16_t, int32_t>> &addrs);
    void restore(engine::Engine &e) override;
    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override;
    std::string describe() const override;
};

// undo of a group move/swap: restore the group-id ordering within a part
struct GroupOrderRestoreItem : public UndoableItem
{
    int16_t part{-1};
    std::vector<GroupID> order;

    void store(engine::Engine &e, int16_t pt);
    void restore(engine::Engine &e) override;
    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override;
    std::string describe() const override;
};

// snapshot of one or more parts' active flag plus their full group
// contents; used for part activate / deactivate which can wipe groups
struct PartsStateItem : public UndoableItem
{
    struct Entry
    {
        int16_t part{-1};
        bool active{false};
        std::vector<std::string> groupsJSON;
    };
    std::vector<Entry> parts;

    void store(engine::Engine &e, const std::vector<int16_t> &whichParts);
    void restore(engine::Engine &e) override;
    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override;
    std::string describe() const override;
};

// full pre-event stream of one part (the SCP payload format); used when a
// part load or a compound import (sf2/sfz/gig/...) replaces the part
struct PartStreamRestoreItem : public UndoableItem
{
    int16_t part{-1};
    std::string label{"Load Part"};
    std::string partJSON;

    void store(engine::Engine &e, int16_t pt, const std::string &lbl);
    void restore(engine::Engine &e) override;
    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override;
    std::string describe() const override;
};

// gesture-aware push of a full part snapshot. Tag is per part so the
// replace-part flow (snapshot, clear, import) coalesces to one undo entry
void pushPartStreamUndo(engine::Engine &e, int16_t part, const std::string &label,
                        UndoGesture g = UndoGesture::Discrete);

// full pre-event engine stream; used when a multi load replaces everything
struct EngineStateRestoreItem : public UndoableItem
{
    std::string engineJSON;

    void store(engine::Engine &e);
    void restore(engine::Engine &e) override;
    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override;
    std::string describe() const override;
};

} // namespace scxt::undo

#endif // SCXT_SRC_SCXT_CORE_UNDO_MANAGER_STRUCTURE_UNDOABLE_ITEMS_H
