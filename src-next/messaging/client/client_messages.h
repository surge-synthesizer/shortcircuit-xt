//
// Created by Paul Walker on 2/5/23.
//

#ifndef SCXT_SRC_MESSAGING_CLIENT_CLIENT_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_CLIENT_MESSAGES_H

#include "messaging/client/detail/client_json_details.h"
#include "json/engine_traits.h"

namespace scxt::messaging::client
{

struct ProcessorDataRequest
{
    static constexpr ClientToSerializationMessagesIds id{c2s_request_processor_data};
    static constexpr bool hasPayload{true};
    typedef engine::Engine::processorAddress_t payload_t;
    typedef std::pair<engine::Engine::processorAddress_t, dsp::processor::ProcessorStorage>
        response_t;

    payload_t payload;

    ProcessorDataRequest(const payload_t &t) : payload(t) {}

    static void executeOnSerialization(const payload_t &address, const engine::Engine &engine,
                                       MessageController &cont)
    {
        // TODO: Check cascade of -1s but for now assume
        const auto &p = engine.getProcessorStorage(address);

        if (p.has_value())
        {
            serializationSendToClient(s2c_respond_processor_data, response_t{address, *p}, cont);
        }
    }
};

template <> struct ClientToSerializationType<ProcessorDataRequest::id>
{
    typedef ProcessorDataRequest T;
};

template <typename Client> struct ProcessorDataResponse
{
    static constexpr SerializationToClientMessageIds id{s2c_respond_processor_data};
    typedef ProcessorDataRequest::response_t payload_t;

    static void executeOnResponse(Client *c, const payload_t &payload)
    {
        const auto &[address, proc] = payload;
        c->onProcessorUpdated(address, proc);
    }
};

template <typename Client> struct SerializationToClientType<s2c_respond_processor_data, Client>
{
    typedef ProcessorDataResponse<Client> T;
};

template <typename Client> struct VoiceCountResponse
{
    static constexpr SerializationToClientMessageIds id{s2c_voice_count};
    typedef uint32_t payload_t;
    static void executeOnResponse(Client *c, const payload_t &payload) { c->onVoiceCount(payload); }
};
template <typename Client> struct SerializationToClientType<s2c_voice_count, Client>
{
    typedef VoiceCountResponse<Client> T;
};

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_CLIENT_MESSAGE_IMPLS_H
