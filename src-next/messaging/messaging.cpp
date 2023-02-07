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
void MessageController::parseAudioMessageOnSerializationThread(
    const audio::AudioToSerialization &as)
{
    // TODO - we could do the template shuffle later if this becomes too big
    switch (as.id)
    {
    case audio::a2s_voice_count:
        client::serializationSendToClient(client::s2c_voice_count, as.payload.u[0], *this);
        break;
    case audio::a2s_pointer_complete:
        returnAudioThreadCallback(static_cast<AudioThreadCallback *>(as.payload.p));
        break;
    case audio::a2s_note_on:
    case audio::a2s_note_off:
        throw std::logic_error("Implement this");
    case audio::a2s_none:
        break;
    }
}

MessageController::~MessageController()
{
    while (!cbStore.empty())
    {
        auto res = cbStore.top();
        delete res;
        cbStore.pop();
    }
}
MessageController::AudioThreadCallback *MessageController::getAudioThreadCallback()
{
    if (cbStore.empty())
    {
        return new AudioThreadCallback();
    }
    auto res = cbStore.top();
    cbStore.pop();
    return res;
}

void MessageController::returnAudioThreadCallback(AudioThreadCallback *r) { cbStore.push(r); }

void MessageController::scheduleAudioThreadCallback(std::function<void(engine::Engine &)> cb)
{
    auto pt = getAudioThreadCallback();
    pt->f = std::move(cb);
    auto s2a = audio::SerializationToAudio();
    s2a.id = audio::s2a_dispatch_to_pointer;
    s2a.payload.p = (void *)pt;
    s2a.payloadType = audio::SerializationToAudio::VOID_STAR;

    serializationToAudioQueue.try_emplace(s2a);
}
} // namespace scxt::messaging