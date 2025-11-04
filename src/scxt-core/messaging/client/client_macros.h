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

#ifndef SCXT_SRC_MESSAGING_CLIENT_CLIENT_MACROS_H
#define SCXT_SRC_MESSAGING_CLIENT_CLIENT_MACROS_H

/*
 * A set of macros useful for generating client messages more compactly
 */

#define CLIENT_TO_SERIAL(className, id, payloadType, executeBody)                                  \
    struct className                                                                               \
    {                                                                                              \
        static constexpr ClientToSerializationMessagesIds c2s_id{id};                              \
        static constexpr bool hasBoundPayload{false};                                              \
        typedef payloadType c2s_payload_t;                                                         \
        c2s_payload_t payload{};                                                                   \
        explicit className(const c2s_payload_t &v) : payload(v) {}                                 \
        static void executeOnSerialization(const c2s_payload_t &payload, engine::Engine &engine,   \
                                           MessageController &cont)                                \
        {                                                                                          \
            executeBody;                                                                           \
        }                                                                                          \
    };                                                                                             \
    template <> struct ClientToSerializationType<className::c2s_id>                                \
    {                                                                                              \
        typedef className T;                                                                       \
    };

#define CLIENT_TO_SERIAL_CONSTRAINED(className, id, payloadType, boundPayload, executeBody)        \
    struct className                                                                               \
    {                                                                                              \
        static constexpr ClientToSerializationMessagesIds c2s_id{id};                              \
        static constexpr bool hasBoundPayload{true};                                               \
        typedef boundPayload bound_t;                                                              \
        typedef payloadType c2s_payload_t;                                                         \
        c2s_payload_t payload{};                                                                   \
        explicit className(const c2s_payload_t &v) : payload(v) {}                                 \
        static void executeOnSerialization(const c2s_payload_t &payload, engine::Engine &engine,   \
                                           MessageController &cont)                                \
        {                                                                                          \
            executeBody;                                                                           \
        }                                                                                          \
    };                                                                                             \
    template <> struct ClientToSerializationType<className::c2s_id>                                \
    {                                                                                              \
        typedef className T;                                                                       \
    };

#define CLIENT_SERIAL_REQUEST_RESPONSE(className, c2id, c2payloadType, s2id, s2payloadType,        \
                                       execSer, cliMethod)                                         \
    struct className                                                                               \
    {                                                                                              \
        static constexpr ClientToSerializationMessagesIds c2s_id{c2id};                            \
        static constexpr SerializationToClientMessageIds s2c_id{s2id};                             \
        static constexpr bool hasBoundPayload{false};                                              \
                                                                                                   \
        typedef c2payloadType c2s_payload_t;                                                       \
        typedef s2payloadType s2c_payload_t;                                                       \
        c2s_payload_t payload{};                                                                   \
        explicit className(const c2s_payload_t &v) : payload(v) {}                                 \
        static void executeOnSerialization(const c2s_payload_t &payload, engine::Engine &engine,   \
                                           MessageController &cont)                                \
        {                                                                                          \
            execSer;                                                                               \
        }                                                                                          \
        template <typename Client>                                                                 \
        static void executeOnClient(Client *c, const s2c_payload_t &payload)                       \
        {                                                                                          \
            c->cliMethod(payload);                                                                 \
        }                                                                                          \
    };                                                                                             \
    template <> struct ClientToSerializationType<className::c2s_id>                                \
    {                                                                                              \
        typedef className T;                                                                       \
    };                                                                                             \
    template <> struct SerializationToClientType<className::s2c_id>                                \
    {                                                                                              \
        typedef className T;                                                                       \
    };

#define SERIAL_TO_CLIENT(className, s2id, s2payloadType, cliMethod)                                \
    struct className                                                                               \
    {                                                                                              \
        static constexpr SerializationToClientMessageIds s2c_id{s2id};                             \
        typedef s2payloadType s2c_payload_t;                                                       \
        template <typename Client>                                                                 \
        static void executeOnClient(Client *c, const s2c_payload_t &payload)                       \
        {                                                                                          \
            c->cliMethod(payload);                                                                 \
        }                                                                                          \
    };                                                                                             \
    template <> struct SerializationToClientType<className::s2c_id>                                \
    {                                                                                              \
        typedef className T;                                                                       \
    };

#endif // SHORTCIRCUIT_CLIENT_MACROS_H
