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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_ENGINESTATUS_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_ENGINESTATUS_MESSAGES_H

#include "messaging/client/client_serial.h"
#include "messaging/client/detail/client_json_details.h"
#include "json/engine_traits.h"
#include "json/datamodel_traits.h"
#include "selection/selection_manager.h"
#include "client_macros.h"

namespace scxt::messaging::client
{

// First in here is: -1 -> show if open, 0 -> close, 1 -> show and open
using activityNotificationPayload_t = std::pair<int, std::string>;
SERIAL_TO_CLIENT(SendActivityNotification, s2c_send_activity_notification,
                 activityNotificationPayload_t, onActivityNotification);

SERIAL_TO_CLIENT(EngineStatusUpdate, s2c_engine_status, engine::Engine::EngineStatusMessage,
                 onEngineStatus);

using unstreamEngineStatePayload_t = std::string;
inline void doUnstreamEngineState(const unstreamEngineStatePayload_t &payload,
                                  engine::Engine &engine, MessageController &cont)
{
    auto mc = messaging::MessageController::ClientActivityNotificationGuard(
        "Unstreaming Engine State", cont);
    if (cont.isAudioRunning)
    {
        cont.stopAudioThreadThenRunOnSerial([payload, &nonconste = engine](auto &e) {
            try
            {
                nonconste.immediatelyTerminateAllVoices();
                scxt::json::unstreamEngineState(nonconste, payload);
                auto &cont = *e.getMessageController();
                cont.restartAudioThreadFromSerial();
                cont.sendStreamCompleteNotification();
            }
            catch (std::exception &err)
            {
                nonconste.getMessageController()->reportErrorToClient(
                    "Unstreaming Error",
                    std::string("Unable to unstream engine state ") + err.what());
            }
        });
    }
    else
    {
        try
        {
            engine.stopEngineRequests++;
            engine.immediatelyTerminateAllVoices();
            scxt::json::unstreamEngineState(engine, payload);
            engine.stopEngineRequests--;
            cont.sendStreamCompleteNotification();
        }
        catch (std::exception &err)
        {
            engine.getMessageController()->reportErrorToClient(
                "Unstreaming Error", std::string("Unable to unstream engine state ") + err.what());
        }
    }
}
CLIENT_TO_SERIAL(UnstreamEngineState, c2s_unstream_engine_state, unstreamEngineStatePayload_t,
                 doUnstreamEngineState(payload, engine, cont));

using stopSounds_t = bool;
inline void stopSoundsMessage(const stopSounds_t &payload, messaging::MessageController &cont)
{
    cont.scheduleAudioThreadCallback([p = payload](scxt::engine::Engine &e) {
        if (p)
            e.voiceManager.allSoundsOff();
        else
            e.voiceManager.allNotesOff();
    });
}
CLIENT_TO_SERIAL(StopSounds, c2s_silence_engine, stopSounds_t, stopSoundsMessage(payload, cont));

using tuningStatusPayload_t =
    std::pair<engine::Engine::TuningMode, engine::Engine::TuningZoneResolution>;
SERIAL_TO_CLIENT(SendTuningStatus, s2c_send_tuning_status, tuningStatusPayload_t, onTuningStatus);

inline void applyTuningStatusPayload(const tuningStatusPayload_t &payload,
                                     messaging::MessageController &cont)
{
    cont.scheduleAudioThreadCallback(
        [p = payload](scxt::engine::Engine &e) {
            e.runtimeConfig.tuningMode = p.first;
            e.runtimeConfig.tuningZoneResolution = p.second;
            e.resetTuningFromRuntimeConfig();
        },
        [](const auto &e) {
            serializationSendToClient(
                messaging::client::s2c_send_tuning_status,
                messaging::client::tuningStatusPayload_t{e.runtimeConfig.tuningMode,
                                                         e.runtimeConfig.tuningZoneResolution},
                *(e.getMessageController()));
        });
}
CLIENT_TO_SERIAL(SetTuningMode, c2s_set_tuning_mode, tuningStatusPayload_t,
                 applyTuningStatusPayload(payload, cont));

using omniFlavor_t = scxt::engine::Engine::OmniFlavor;
inline void applyOmniFlavorPayload(const omniFlavor_t &payload, messaging::MessageController &cont)
{
    cont.scheduleAudioThreadCallback(
        [payload](scxt::engine::Engine &e) { e.setOmniFlavor(payload); });
}
CLIENT_TO_SERIAL(SetOmniFlavor, c2s_set_omni_flavor, omniFlavor_t,
                 applyOmniFlavorPayload(payload, cont));

using omniFlavorUpdate_t = std::pair<int, bool>;
SERIAL_TO_CLIENT(UpdateOmniFlavor, s2c_update_omni_flavor, omniFlavorUpdate_t,
                 onOmniFlavorFromEngine);

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_ENGINESTATUS_MESSAGES_H
