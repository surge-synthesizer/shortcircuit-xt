//
// Created by Paul Walker on 2/6/23.
//

#ifndef SCXT_SRC_MESSAGING_AUDIO_AUDIO_MESSAGES_H
#define SCXT_SRC_MESSAGING_AUDIO_AUDIO_MESSAGES_H

#include "audio_serial.h"
#include "engine/engine.h"

namespace scxt::messaging::audio
{
// Audio thread
void sendVoiceCount(uint32_t voiceCount, MessageController &mc);

} // namespace scxt::messaging::audio
#endif // SHORTCIRCUIT_AUDIO_MESSAGES_H
