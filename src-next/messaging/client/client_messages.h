/*
 * ShortCircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
 * transaction log.
 *
 * ShortCircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortCircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortCircuitXT is inspired by, and shares some code with, the
 * commercial product ShortCircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for ShortCircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortCircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

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
