/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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
                SCLOG("Unable to unstream [" << err.what() << "]");
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
            SCLOG("Unable to unstream [" << err.what() << "]");
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

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_ENGINESTATUS_MESSAGES_H
