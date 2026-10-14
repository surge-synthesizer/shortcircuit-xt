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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_HARNESS_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_HARNESS_MESSAGES_H

#include "client_macros.h"
#include "client_serial.h"
#include "engine/engine.h"

namespace scxt::messaging::client
{
// test clients wait on this; the plugin never sends it and has no handler for the reply
SERIAL_TO_CLIENT_OPT_IN(HarnessSyncReached, s2c_harness_sync, int64_t, onHarnessSync);

inline void replyAfterAudioTrips(int64_t token, int trips, MessageController &cont)
{
    if (trips == 0)
    {
        serializationSendToClient(s2c_harness_sync, token, cont);
        return;
    }
    cont.scheduleAudioThreadCallback(
        [](auto &) {},
        [token, trips, &cont](const auto &) { replyAfterAudioTrips(token, trips - 1, cont); });
}

// two trips, so audio side effects of earlier messages are back on serial first
CLIENT_TO_SERIAL(HarnessSync, c2s_harness_sync, int64_t, replyAfterAudioTrips(payload, 2, cont));
} // namespace scxt::messaging::client

#endif // SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_HARNESS_MESSAGES_H
