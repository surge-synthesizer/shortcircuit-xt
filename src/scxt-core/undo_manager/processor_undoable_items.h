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

#ifndef SCXT_SRC_SCXT_CORE_UNDO_MANAGER_PROCESSOR_UNDOABLE_ITEMS_H
#define SCXT_SRC_SCXT_CORE_UNDO_MANAGER_PROCESSOR_UNDOABLE_ITEMS_H

#include "undoable_items.h"
#include "dsp/processor/processor.h"

namespace scxt::undo
{

struct ZoneProcessorTypeChangeItem : public MultiSelectUndoBaseItem
{
    int32_t whichProcessor{-1};
    std::vector<dsp::processor::ProcessorStorage> cachedStorage;

    void store(engine::Engine &e, int32_t which,
               const std::vector<selection::SelectionManager::ZoneAddress> &zones);
    void restore(engine::Engine &e) override;
    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override;
    std::string describe() const override;
};

struct GroupProcessorTypeChangeItem : public MultiSelectUndoBaseItem
{
    int32_t whichProcessor{-1};
    std::vector<dsp::processor::ProcessorStorage> cachedStorage;

    void store(engine::Engine &e, int32_t which,
               const std::vector<selection::SelectionManager::ZoneAddress> &groups);
    void restore(engine::Engine &e) override;
    std::unique_ptr<UndoableItem> makeRedo(engine::Engine &e) override;
    std::string describe() const override;
};

} // namespace scxt::undo

#endif // SCXT_SRC_SCXT_CORE_UNDO_MANAGER_PROCESSOR_UNDOABLE_ITEMS_H
