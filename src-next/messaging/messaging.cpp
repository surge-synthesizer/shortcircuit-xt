//
// Created by Paul Walker on 2/6/23.
//

#include "messaging/messaging.h"
#include "messaging/audio/audio_serial.h"
#include "client/client_serial.h"
#include "client/client_serial_impl.h"
#include "client/client_messages.h"

namespace scxt::messaging
{
// Serialization Thread
void MessageController::parseAudioMessageAndSendToClient(const audio::AudioToSerialization &as)
{
    // TODO - we could do the template shuffle later if this becomes too big
    switch (as.id)
    {
    case audio::a2s_voice_count:
        client::serializationSendToClient(client::s2c_voice_count, as.payload.u[0], *this);
        break;
    case audio::a2s_note_on:
    case audio::a2s_note_off:
        throw std::logic_error("Implement this");
    case audio::a2s_none:
        break;
    }
}
} // namespace scxt::messaging