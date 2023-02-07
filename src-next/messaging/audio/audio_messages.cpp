//
// Created by Paul Walker on 2/6/23.
//

#include <iostream>

#include "audio_messages.h"
#include "messaging/messaging.h"

namespace scxt::messaging::audio
{
void sendVoiceCount(uint32_t voiceCount, MessageController &mc)
{
    AudioToSerialization a2s;
    a2s.id = a2s_voice_count;
    a2s.payloadType = AudioToSerialization::UINT;
    a2s.payload.u[0] = voiceCount;
    mc.sendAudioToSerialization(a2s);
}

} // namespace scxt::messaging::audio