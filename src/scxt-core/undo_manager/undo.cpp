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

#include "undo.h"

#include "engine/engine.h"
#include "messaging/messaging.h"
#include "utils.h"

namespace scxt::undo
{

void UndoManager::storeUndoStep(std::unique_ptr<UndoableItem> item)
{
    if (coalesce == Coalesce::Captured)
    {
        SCLOG_IF(undoRedo, "|== drop '" << item->describe() << "' coalesced into open batch");
        return;
    }
    SCLOG_IF(undoRedo, "|>> push '" << item->describe() << "' (stack size " << undoStack.size() + 1
                                    << "), clearing redo stack");
    closeGesture();
    redoStack.clear();
    undoStack.push_back(std::move(item));
    while (undoStack.size() > maxUndoRedoStackSize)
        undoStack.pop_front();
    if (coalesce == Coalesce::Armed)
        coalesce = Coalesce::Captured;
}

void UndoManager::storeUndoStepTagged(std::unique_ptr<UndoableItem> item, const std::string &tag,
                                      UndoGesture g)
{
    if (g == UndoGesture::Discrete && gestureCovers(tag))
    {
        SCLOG_IF(undoRedo,
                 "|== drop '" << item->describe() << "' covered by open gesture '" << tag << "'");
        return;
    }
    storeUndoStep(std::move(item));
    if (g == UndoGesture::Begin)
        openGesture(tag);
}

void UndoManager::clear()
{
    SCLOG_IF(undoRedo, "|xx clearing undo (" << undoStack.size() << ") and redo ("
                                             << redoStack.size() << ") stacks");
    undoStack.clear();
    redoStack.clear();
    closeGesture();
    endCoalescing();
}

bool UndoManager::applyUndoStep(engine::Engine &e)
{
    assert(e.getMessageController()->threadingChecker.isSerialThread());
    closeGesture();
    endCoalescing();
    if (undoStack.empty())
        return false;

    auto item = std::move(undoStack.back());
    undoStack.pop_back();

    SCLOG_IF(undoRedo, "|<< restoring '" << item->describe() << "' (undo stack size "
                                         << undoStack.size() << ")");

    auto redoItem = item->makeRedo(e);
    if (redoItem)
    {
        SCLOG_IF(undoRedo, "   |>> redo '" << redoItem->describe() << "'");
        redoStack.push_back(std::move(redoItem));
        while (redoStack.size() > maxUndoRedoStackSize)
            redoStack.pop_front();
    }

    item->restore(e);
    return true;
}

bool UndoManager::applyRedoStep(engine::Engine &e)
{
    assert(e.getMessageController()->threadingChecker.isSerialThread());
    closeGesture();
    endCoalescing();
    if (redoStack.empty())
        return false;

    auto item = std::move(redoStack.back());
    redoStack.pop_back();

    SCLOG_IF(undoRedo, "|<< Redo:- restoring '" << item->describe() << "' (redo stack size "
                                                << redoStack.size() << ")");

    auto undoItem = item->makeRedo(e);
    if (undoItem)
    {
        SCLOG_IF(undoRedo, "   |++ Redo: pushing undo '" << undoItem->describe() << "'");
        undoStack.push_back(std::move(undoItem));
        while (undoStack.size() > maxUndoRedoStackSize)
            undoStack.pop_front();
    }

    item->restore(e);
    return true;
}

} // namespace scxt::undo
