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

#ifndef SCXT_SRC_SCXT_CORE_UNDO_MANAGER_UNDO_H
#define SCXT_SRC_SCXT_CORE_UNDO_MANAGER_UNDO_H

#include <memory>
#include <deque>
#include <optional>
#include <string>
#include "undoable_items.h"

namespace scxt::undo
{

// Begin opens a gesture: while it is open, Discrete pushes with the same
// tag are dropped — undo is per event, not per change
enum struct UndoGesture
{
    Discrete,
    Begin
};

struct UndoManager
{
    void storeUndoStep(std::unique_ptr<UndoableItem> item);
    // gesture-aware push: drops the item if a gesture with this tag is
    // open; any other tag closes that gesture first. Begin opens the tag.
    void storeUndoStepTagged(std::unique_ptr<UndoableItem> item, const std::string &tag,
                             UndoGesture g);
    bool applyUndoStep(engine::Engine &e);
    bool applyRedoStep(engine::Engine &e);
    // For whole-engine replacements (load multi, load part, unstream, reset)
    // where prior items reference state which no longer exists
    void clear();
    bool hasUndoSteps() const { return !undoStack.empty(); }
    bool hasRedoSteps() const { return !redoStack.empty(); }
    size_t undoStackSize() const { return undoStack.size(); }
    size_t redoStackSize() const { return redoStack.size(); }

    /*
     * Undo is per event, not per change. A continuous gesture (knob drag,
     * zone drag) opens a tag at begin-edit; while it is open, handler-side
     * pushes carrying the same tag are dropped since the begin snapshot
     * already covers the whole gesture. End-edit (or any undo/redo/clear)
     * closes it.
     */
    void openGesture(const std::string &tag) { openGestureTag = tag; }
    void closeGesture() { openGestureTag = std::nullopt; }
    bool gestureCovers(const std::string &tag) const
    {
        return openGestureTag.has_value() && *openGestureTag == tag;
    }

  private:
    std::deque<std::unique_ptr<UndoableItem>> undoStack;
    std::deque<std::unique_ptr<UndoableItem>> redoStack;
    std::optional<std::string> openGestureTag;
};

} // namespace scxt::undo

#endif // SCXT_SRC_SCXT_CORE_UNDO_MANAGER_UNDO_H
