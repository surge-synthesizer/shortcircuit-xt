//
// Created by Paul Walker on 2/5/23.
//

#ifndef SCXT_SRC_MESSAGING_CLIENT_CLIENT_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_CLIENT_MESSAGES_H

#include "client_json_details.h"
#include "json/engine_traits.h"

namespace scxt::messaging::client
{
struct RefreshPatchRequest
{
    static constexpr ClientToSerializationMessagesIds id{c2s_refresh_patch};
    static constexpr bool hasState{false};

    void executeOnSerialization(const engine::Engine &engine, MessageController &cont)
    {
        serializationSendToClient(s2c_patch_stream, *(engine.getPatch()), cont);
    }
};

template <> struct ClientToSerializationType<c2s_refresh_patch>
{
    typedef RefreshPatchRequest T;
};

template <typename Client> struct PatchStreamResponse
{
    static constexpr SerializationToClientMessageIds id{s2c_patch_stream};
    typedef engine::Patch payload_t;

    static void executeOnResponse(Client *c, const payload_t &payload)
    {
        c->onPatchStreamed(payload);
    }
};

template <typename Client> struct SerializationClientType<s2c_patch_stream, Client>
{
    typedef PatchStreamResponse<Client> T;
};

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_CLIENT_MESSAGE_IMPLS_H
