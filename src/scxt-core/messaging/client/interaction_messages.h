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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_INTERACTION_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_INTERACTION_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/selection_traits.h"
#include "selection/selection_manager.h"
#include "engine/engine.h"
#include "client_macros.h"
#include "patch_io/patch_io.h"
#include "utils.h"

namespace scxt::messaging::client
{
// title, message, source file, source line
typedef std::tuple<std::string, std::string, std::string, int> s2cError_t;
SERIAL_TO_CLIENT(ReportError, s2c_report_error, s2cError_t, onErrorFromEngine);

inline void raiseDebugError(MessageController &c, int count)
{
    for (int i = 0; i < count; ++i)
    {
        RAISE_ERROR_CONT(c, "A Dummy Error " + std::to_string(i),
                         "This is a dummy error " + std::to_string(i) +
                             ". I chose to have it have "
                             "a very long message like this one so I can test multiline "
                             "string rendering in the error box. So this one has details "
                             "like "
                             "this and that");
    }
}
CLIENT_TO_SERIAL(RaiseDebugError, c2s_raise_debug_error, int, raiseDebugError(cont, payload))

// note, 0...1 velocity, onoff
typedef std::tuple<int32_t, float, bool> noteOnOff_t;
inline void processMidiFromGUI(const noteOnOff_t &g, const engine::Engine &engine,
                               MessageController &cont)
{
    auto [n, v, onoff] = g;

    auto sel = engine.getSelectionManager()->selectedPart;
    if (sel < 0)
        return;

    auto p = sel;
    auto ch = engine.getPatch()->getPart(p)->configuration.channel;
    if (ch < 0)
        ch = 0;

    if (onoff)
    {
        cont.scheduleAudioThreadCallback([ch, vel = v, note = n](auto &eng) {
            eng.voiceManager.processNoteOnEvent(0, ch, note, -1, vel, 0.f);
        });
    }
    else
    {
        cont.scheduleAudioThreadCallback([ch, vel = v, note = n](auto &eng) {
            eng.voiceManager.processNoteOffEvent(0, ch, note, -1, vel);
        });
    }
}
CLIENT_TO_SERIAL(NoteFromGUI, c2s_noteonoff, noteOnOff_t, processMidiFromGUI(payload, engine, cont))

inline void doHostCallback(uint64_t pl, MessageController &cont)
{
    if (cont.requestHostCallback)
    {
        cont.requestHostCallback(pl);
    }
}
CLIENT_TO_SERIAL(RequestHostCallback, c2s_request_host_callback, uint64_t,
                 doHostCallback(payload, cont));

inline void doResetEngine(const std::string &fl, engine::Engine &e, MessageController &cont)
{
    scxt::patch_io::initFromResourceBundle(e, fl);
    e.sendFullRefreshToClient();
}
CLIENT_TO_SERIAL(ResetEngine, c2s_reset_engine, std::string, doResetEngine(payload, engine, cont));

inline void doResendFullState(const bool &b, engine::Engine &e, MessageController &cont)
{
    if (b)
    {
        e.sendFullRefreshToClient();
    }
    else
    {
        SCLOG_IF(debug, "Why did you bother sending the resend false message?");
    }
}
CLIENT_TO_SERIAL(ResendFullState, c2s_resend_full_state, bool,
                 doResendFullState(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_INTERACTION_MESSAGES_H
