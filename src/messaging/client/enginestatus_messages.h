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

#ifndef SCXT_SRC_MESSAGING_CLIENT_ENGINESTATUS_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_ENGINESTATUS_MESSAGES_H

#include "messaging/client/client_serial.h"
#include "messaging/client/detail/client_json_details.h"
#include "json/engine_traits.h"
#include "json/datamodel_traits.h"
#include "selection/selection_manager.h"
#include "client_macros.h"

namespace scxt::messaging::client
{
SERIAL_TO_CLIENT(EngineStatusUpdate, s2c_engine_status, engine::Engine::EngineStatusMessage,
                 onEngineStatus);

using streamState_t = std::string;
inline void onUnstream(const streamState_t &payload, engine::Engine &engine,
                       MessageController &cont)
{
    if (cont.isAudioRunning)
    {
        cont.stopAudioThreadThenRunOnSerial([payload, &nonconste = engine](auto &e) {
            try
            {
                scxt::json::unstreamEngineState(nonconste, payload);
                auto &cont = *e.getMessageController();
                cont.restartAudioThreadFromSerial();
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
            scxt::json::unstreamEngineState(engine, payload);
        }
        catch (std::exception &err)
        {
            SCLOG("Unable to unstream [" << err.what() << "]");
        }
    }
}
CLIENT_TO_SERIAL(UnstreamIntoEngine, c2s_unstream_state, streamState_t,
                 onUnstream(payload, engine, cont));

using stopSounds_t = bool;
inline void stopSoundsMessage(const stopSounds_t &payload, messaging::MessageController &cont)
{
    cont.scheduleAudioThreadCallback([p = payload](scxt::engine::Engine &e) {
        if (p)
            e.stopAllSounds();
        else
            e.releaseAllVoices();
    });
}
CLIENT_TO_SERIAL(StopSounds, c2s_silence_engine, stopSounds_t, stopSoundsMessage(payload, cont));
} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_ENGINESTATUS_MESSAGES_H
