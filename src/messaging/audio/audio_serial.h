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

#ifndef SCXT_SRC_MESSAGING_AUDIO_AUDIO_SERIAL_H
#define SCXT_SRC_MESSAGING_AUDIO_AUDIO_SERIAL_H

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
