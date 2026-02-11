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

// Begin and End Edit messages. These are a mess. See #775
using editGestureFor_t = bool;
inline void doBeginEndEdit(bool isBegin, const editGestureFor_t &payload,
                           const engine::Engine &engine, messaging::MessageController &cont)
{
    if (!isBegin)
    {
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
            engine.getSelectionManager()->configureAndSendZoneOrGroupModMatrixMetadata(
                lg->part, lg->group, -1);
        }
    }
}
CLIENT_TO_SERIAL(BeginEdit, c2s_begin_edit, editGestureFor_t,
                 doBeginEndEdit(true, payload, engine, cont));
CLIENT_TO_SERIAL(EndEdit, c2s_end_edit, editGestureFor_t,
                 doBeginEndEdit(false, payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_SELECTION_MESSAGES_H
