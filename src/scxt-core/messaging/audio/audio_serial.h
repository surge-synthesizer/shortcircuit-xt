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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_AUDIO_AUDIO_SERIAL_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_AUDIO_AUDIO_SERIAL_H

#include <cstdint>
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
    a2s_note_on,
    a2s_note_off,
    a2s_structure_refresh,
    a2s_processor_refresh,
    a2s_macro_updated,
    a2s_delete_this_pointer,
    a2s_schedule_sample_purge
};

/**
 * Message originated from the audio thread
 */
struct AudioToSerialization
{
    AudioToSerializationMessageId id{a2s_none};

    // std::variant does not allow an array type and
    // i kinda want this fixed size anyway
    struct ToBeDeleted
    {
        void *ptr;
        enum uint16_t
        {
            engine_Zone,
            engine_Group,
        } type;
    };

    static_assert(sizeof(ToBeDeleted) < 8 * sizeof(int32_t));
    union Payload
    {
        int32_t i[8];
        uint32_t u[8];
        float f[8];
        char c[32];
        void *p;
        ToBeDeleted delThis;
    } payload{};

    enum PayloadType : uint8_t
    {
        NONE,
        INT,
        UINT,
        FLOAT,
        CHAR,
        VOID_STAR,
        TO_BE_DELETED
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
    s2a_dispatch_to_pointer,
    s2a_dispatch_to_pointer_under_structurelock,

    s2a_param_beginendedit,
    s2a_param_set_value,
    s2a_param_refresh
};

/**
 * Message originated from the audio thread
 */
struct SerializationToAudio
{
    SerializationToAudioMessageId id{s2a_none};

    // std::variant does not allow an array type and
    // i kinda want this fixed size anyway
    struct FourUintsFourFloats
    {
        uint32_t u[4];
        float f[4];
    };
    union Payload
    {
        int32_t i[8];
        uint32_t u[8];
        float f[8];
        char c[32];
        void *p;
        FourUintsFourFloats mix;
    } payload{};

    enum PayloadType : uint8_t
    {
        NONE,
        INT,
        UINT,
        FLOAT,
        CHAR,
        VOID_STAR,
        FOUR_FOUR_MIX
    } payloadType{INT};
};

} // namespace scxt::messaging::audio

#endif // SHORTCIRCUIT_AUDIO_SERIAL_H
