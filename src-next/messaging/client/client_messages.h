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
#include "json/datamodel_traits.h"

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

struct AdsrSelectedZoneViewRequest
{
    static constexpr ClientToSerializationMessagesIds id{c2s_request_zone_adsr_view};
    static constexpr bool hasPayload{true};
    typedef int payload_t;
    typedef std::tuple<int, datamodel::AdsrStorage> response_t;

    payload_t payload;

    AdsrSelectedZoneViewRequest(int whichEnv) : payload(whichEnv) {}

    static void executeOnSerialization(const payload_t &which, const engine::Engine &engine,
                                       MessageController &cont)
    {
        // TODO Selected Zone State
        const auto &selectedZone = engine.getPatch()->getPart(0)->getGroup(0)->getZone(0);

        if (which == 0)
            serializationSendToClient(s2c_respond_zone_adsr_view,
                                      response_t{which, selectedZone->aegStorage}, cont);
        if (which == 1)
            serializationSendToClient(s2c_respond_zone_adsr_view,
                                      response_t{which, selectedZone->eg2Storage}, cont);
    }
};

template <> struct ClientToSerializationType<AdsrSelectedZoneViewRequest::id>
{
    typedef AdsrSelectedZoneViewRequest T;
};

template <typename Client> struct AdsrSelectedZoneViewResponse
{
    static constexpr SerializationToClientMessageIds id{s2c_respond_zone_adsr_view};
    typedef AdsrSelectedZoneViewRequest::response_t payload_t;

    static void executeOnResponse(Client *c, const payload_t &payload)
    {
        const auto &[which, env] = payload;
        c->onEnvelopeUpdated(which, env);
    }
};

template <typename Client> struct SerializationToClientType<s2c_respond_zone_adsr_view, Client>
{
    typedef AdsrSelectedZoneViewResponse<Client> T;
};

struct AdsrSelectedZoneUpdateRequest
{
    static constexpr ClientToSerializationMessagesIds id{c2s_update_zone_adsr_view};
    static constexpr bool hasPayload{true};
    typedef std::tuple<int, datamodel::AdsrStorage> payload_t;

    payload_t payload;

    AdsrSelectedZoneUpdateRequest(int whichEnv, const datamodel::AdsrStorage &a)
        : payload{whichEnv, a}
    {
    }

    static void executeOnSerialization(const payload_t &payload, const engine::Engine &engine,
                                       MessageController &cont)
    {
        // TODO Selected Zone State
        const auto &[e, adsr] = payload;
        cont.scheduleAudioThreadCallback([ew = e, adsrv = adsr](auto &eng) {
            if (ew == 0)
                eng.getPatch()->getPart(0)->getGroup(0)->getZone(0)->aegStorage = adsrv;
        });
    }
};

template <> struct ClientToSerializationType<AdsrSelectedZoneUpdateRequest::id>
{
    typedef AdsrSelectedZoneUpdateRequest T;
};

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_CLIENT_MESSAGE_IMPLS_H
