/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

typedef std::pair<std::optional<selection::SelectionManager::ZoneAddress>,
                  selection::SelectionManager::selectedZones_t>
    selectedStateMessage_t;
SERIAL_TO_CLIENT(SetSelectionState, s2c_send_selection_state, selectedStateMessage_t,
                 onSelectionState);

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_SELECTION_MESSAGES_H
