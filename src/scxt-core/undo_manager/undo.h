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
    void storeUndoStep(engine::Engine &e, std::unique_ptr<UndoableItem> item);
    // gesture-aware push: drops the item if a gesture with this tag is
    // open; any other tag closes that gesture first. Begin opens the tag.
    void storeUndoStepTagged(engine::Engine &e, std::unique_ptr<UndoableItem> item,
                             const std::string &tag, UndoGesture g);
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

    /*
     * Coalesce a batch of pushes into one undo entry. A UI loop (e.g. dropping
     * several samples, which fires one add message per file) brackets itself
     * with begin/endCoalescing; the first push during the batch is kept and the
     * rest dropped. Since the bulk handlers snapshot before their deferred
     * writes, that first item captures the pre-batch state, so one undo reverts
     * the whole batch. Tag-agnostic, so it works across handlers that snapshot
     * different subtrees (group add vs zone variants).
     */
    void beginCoalescing() { coalesce = Coalesce::Armed; }
    void endCoalescing() { coalesce = Coalesce::Off; }

  private:
    std::deque<std::unique_ptr<UndoableItem>> undoStack;
    std::deque<std::unique_ptr<UndoableItem>> redoStack;
    std::optional<std::string> openGestureTag;

    enum struct Coalesce
    {
        Off,
        Armed,    // batch open, no entry kept yet
        Captured, // first entry kept; drop the rest until endCoalescing
    } coalesce{Coalesce::Off};
};

// Construct an undo Item, store() the pre-event state into it, and push it.
// store() is always (engine, ...rest), so the rest forwards through. The
// engine type is a template parameter so the .undoManager access is dependent
// and resolved at instantiation - undo.h is pulled in by engine.h while Engine
// is still incomplete.
template <typename Item, typename E, typename... Args> void pushUndo(E &e, Args &&...args)
{
    auto item = std::make_unique<Item>();
    item->store(e, std::forward<Args>(args)...);
    e.undoManager.storeUndoStep(e, std::move(item));
}

// Gesture-aware variant: tag + gesture so continuous edits coalesce to one entry.
template <typename Item, typename E, typename... Args>
void pushUndoTagged(E &e, const std::string &tag, UndoGesture g, Args &&...args)
{
    auto item = std::make_unique<Item>();
    item->store(e, std::forward<Args>(args)...);
    e.undoManager.storeUndoStepTagged(e, std::move(item), tag, g);
}

} // namespace scxt::undo

#endif // SCXT_SRC_SCXT_CORE_UNDO_MANAGER_UNDO_H
