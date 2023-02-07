//
// Created by Paul Walker on 2/6/23.
//

#ifndef SCXT_SRC_MESSAGING_AUDIO_AUDIO_SERIAL_H
#define SCXT_SRC_MESSAGING_AUDIO_AUDIO_SERIAL_H

#include <string>
#include <sstream>

namespace scxt::messaging
{
struct MessageController;
}

namespace scxt::messaging::audio
{
/**
 * We need fixed size data structures to communicate audio <> serialization
 * thread basically
 */

enum AudioToSerializationMessageId
{
    a2s_none,
    a2s_pointer_complete,
    a2s_voice_count,
    a2s_note_on,
    a2s_note_off
};

/**
 * Message originated from the audio thread
 */
struct AudioToSerialization
{
    AudioToSerializationMessageId id{a2s_none};

    // std::variant does not allow an array type and
    // i kinda want this fixed size anyway
    union Payload
    {
        int32_t i[8];
        uint32_t u[8];
        float f[8];
        char c[32];
        void *p;
    } payload{};

    enum PayloadType : uint8_t
    {
        INT,
        UINT,
        FLOAT,
        CHAR,
        VOID_STAR
    } payloadType{INT};
};

inline std::string toDebugString(const AudioToSerialization &a)
{
    std::ostringstream oss;
    oss << "A2S " << a.id << " " << a.payloadType;
    return oss.str();
}

enum SerializationToAudioMessageId
{
    s2a_none,
    s2a_dispatch_to_pointer
};

/**
 * Message originated from the audio thread
 */
struct SerializationToAudio
{
    SerializationToAudioMessageId id{s2a_none};

    // std::variant does not allow an array type and
    // i kinda want this fixed size anyway
    union Payload
    {
        int32_t i[8];
        uint32_t u[8];
        float f[8];
        char c[32];
        void *p;
    } payload{};

    enum PayloadType : uint8_t
    {
        INT,
        UINT,
        FLOAT,
        CHAR,
        VOID_STAR
    } payloadType{INT};
};

} // namespace scxt::messaging::audio

#endif // SHORTCIRCUIT_AUDIO_SERIAL_H
