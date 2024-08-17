/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#ifndef SCXT_SRC_MESSAGING_CLIENT_SELECTION_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_SELECTION_MESSAGES_H

#include "client_macros.h"
#include "messaging/messaging.h"
#include "selection/selection_manager.h"

namespace scxt::messaging::client
{

inline void doSelectAction(const selection::SelectionManager::SelectActionContents &za,
                           const engine::Engine &engine)
{
    engine.getSelectionManager()->selectAction(za);
}
CLIENT_TO_SERIAL(DoSelectAction, c2s_do_select_action,
                 selection::SelectionManager::SelectActionContents,
                 doSelectAction(payload, engine));

inline void
doMultiSelectAction(const std::vector<selection::SelectionManager::SelectActionContents> &za,
                    const engine::Engine &engine)
{
    engine.getSelectionManager()->multiSelectAction(za);
}
CLIENT_TO_SERIAL(DoMultiSelectAction, c2s_do_multi_select_action,
                 std::vector<selection::SelectionManager::SelectActionContents>,
                 doMultiSelectAction(payload, engine));

CLIENT_TO_SERIAL(DoSelectPart, c2s_select_part, int16_t,
                 engine.getSelectionManager()->selectPart(payload));

// Lead Zone, Zone Selection, Gropu Selection
typedef std::tuple<std::optional<selection::SelectionManager::ZoneAddress>,
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
            engine.getSelectionManager()->configureAndSendZoneModMatrixMetadata(lz->part, lz->group,
                                                                                lz->zone);
        }
    }
}
CLIENT_TO_SERIAL(BeginEdit, c2s_begin_edit, editGestureFor_t,
                 doBeginEndEdit(true, payload, engine, cont));
CLIENT_TO_SERIAL(EndEdit, c2s_end_edit, editGestureFor_t,
                 doBeginEndEdit(false, payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_SELECTION_MESSAGES_H
