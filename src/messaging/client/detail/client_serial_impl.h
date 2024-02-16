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

#ifndef SCXT_SRC_MESSAGING_CLIENT_DETAIL_CLIENT_SERIAL_IMPL_H
#define SCXT_SRC_MESSAGING_CLIENT_DETAIL_CLIENT_SERIAL_IMPL_H

#include "tao/json/to_string.hpp"
#include "tao/json/from_string.hpp"

#include "tao/json/msgpack/consume_string.hpp"
#include "tao/json/msgpack/from_binary.hpp"
#include "tao/json/msgpack/from_input.hpp"
#include "tao/json/msgpack/from_string.hpp"
#include "tao/json/msgpack/parts_parser.hpp"
#include "tao/json/msgpack/to_stream.hpp"
#include "tao/json/msgpack/to_string.hpp"

#include "messaging/client/detail/client_json_details.h"

// This is a 'details only' file which you can safely ignore
// once it works, basically.
namespace scxt::messaging::client
{
/*
 * Inter-process messages can pack as JSON (the streaming format we use)
 * or as msgpack messages which are a lot smaller and faster. PROCESS_AS_JSON
 * is really just useful for debugging where you actually want to see a message
 */
#define PROCESS_AS_JSON 0
#if PROCESS_AS_JSON
namespace encoder = tao::json;
#else
namespace encoder = tao::json::msgpack;
#endif

namespace detail
{
template <typename T> struct MessageWrapper
{
    const T &msg;
    MessageWrapper(const T &m) : msg(m) {}
};
template <typename T> struct client_message_traits<MessageWrapper<T>>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const MessageWrapper<T> &t)
    {
        auto iid = (int)T::c2s_id;
        client_message_value vt = t.msg.payload;
        v = {{"id", iid}, {"object", vt}};
    }
};

template <typename T> struct ResponseWrapper
{
    const T &msg;
    scxt::messaging::client::SerializationToClientMessageIds id;
    ResponseWrapper(const T &m, scxt::messaging::client::SerializationToClientMessageIds i)
        : msg(m), id(i)
    {
    }
};
template <typename T> struct client_message_traits<ResponseWrapper<T>>
{
    template <template <typename...> class Traits>
    static void assign(tao::json::basic_value<Traits> &v, const ResponseWrapper<T> &t)
    {
        auto iid = (int)t.id;
        v = {{"id", iid}, {"object", t.msg}};
    }
};

template <size_t I, template <typename...> class Traits>
void doExecOnSerialization(tao::json::basic_value<Traits> &o, engine::Engine &e,
                           MessageController &mc)
{
    if constexpr (std::is_same<
                      typename ClientToSerializationType<(ClientToSerializationMessagesIds)I>::T,
                      unimpl_t>::value)
    {
        // If you hit this assert you have not specialized ClientToSerializationType for the ID
        assert(false);
        return;
    }
    else
    {
        typedef
            typename ClientToSerializationType<(ClientToSerializationMessagesIds)I>::T handler_t;

        typename handler_t::c2s_payload_t payload;
        client_message_value mv = o.get_object()["object"];
        mv.to(payload);
        handler_t::executeOnSerialization(payload, e, mc);
    }
}

template <template <typename...> class Traits, size_t... Is>
auto executeOnSerializationFor(size_t ft, typename tao::json::basic_value<Traits> &o,
                               engine::Engine &e, MessageController &mc, std::index_sequence<Is...>)
{
    using vtOp_t =
        void (*)(tao::json::basic_value<Traits> &, engine::Engine &, MessageController &);
    constexpr vtOp_t fnc[] = {detail::doExecOnSerialization<Is>...};
    return fnc[ft](o, e, mc);
}

template <size_t I, typename Client, template <typename...> class Traits>
void doExecOnClient(tao::json::basic_value<Traits> &o, Client *c)
{
    typedef typename SerializationToClientType<(SerializationToClientMessageIds)I>::T handler_t;
    if constexpr (std::is_same<handler_t, unimpl_t>::value)
    {
        // If you hit this assert, it means you have not defined a handler type, probably
        // skipping the SERIAL_TO_CLIENT
        assert(false);
        return;
    }
    else
    {
        typename handler_t::s2c_payload_t payload;

        client_message_value mv = o.get_object()["object"];
        mv.to(payload);
        handler_t::template executeOnClient<Client>(c, payload);
    }
}
template <typename Client, template <typename...> class Traits, size_t... Is>
auto executeOnClientFor(size_t ft, typename tao::json::basic_value<Traits> &o, Client *c,
                        std::index_sequence<Is...>)
{
    using vtOp_t = void (*)(tao::json::basic_value<Traits> &, Client *);
    constexpr vtOp_t fnc[] = {detail::doExecOnClient<Is>...};
    return fnc[ft](o, c);
}

} // namespace detail

template <typename T>
inline void clientSendToSerialization(const T &msg, messaging::MessageController &mc)
{
    assert(mc.threadingChecker.isClientThread());
    auto mw = detail::MessageWrapper(msg);
    detail::client_message_value v = mw;
    auto res = encoder::to_string(v);
#if BUILD_IS_DEBUG
    mc.c2sMessageCount++;
    mc.c2sMessageBytes += res.size();
    if (mc.c2sMessageCount % 100 == 0)
    {
        SCLOG("Client -> Serial Message Count : " << mc.c2sMessageCount << " size "
                                                  << mc.c2sMessageBytes << " avgmsg: "
                                                  << 1.f * mc.c2sMessageBytes / mc.c2sMessageCount);
    }
#endif
    mc.sendRawFromClient(res);
}

template <typename T>
inline void serializationSendToClient(SerializationToClientMessageIds id, const T &msg,
                                      messaging::MessageController &mc)
{
    assert(mc.threadingChecker.isSerialThread());

    try
    {
        // TODO - consider waht to do here. Dropping the message is probably best
        if (!mc.clientCallback)
        {
            if (cacheSerializationMessagesAbsentClient(id))
            {
                auto mw = detail::ResponseWrapper<T>(msg, id);
                detail::client_message_value v = mw;
                auto res = encoder::to_string(v);
                mc.preClientConnectionCache.push_back(res);
            }
            return;
        }

        auto mw = detail::ResponseWrapper<T>(msg, id);
        detail::client_message_value v = mw;
        auto res = encoder::to_string(v);
        mc.clientCallback(res);
    }
    catch (const std::exception &e)
    {
        if (id != s2c_report_error)
        {
            mc.reportErrorToClient("JSON Streaming Error", e.what());
        }
    }
}

inline void serializationThreadExecuteClientMessage(const std::string &msgView, engine::Engine &e,
                                                    MessageController &mc)
{
    assert(mc.threadingChecker.isSerialThread());
    using namespace tao::json;

    // We need to unpack with the client message traits
    events::transformer<events::to_basic_value<detail::client_message_traits>> consumer;
    encoder::events::from_string(consumer, msgView);
    auto jv = std::move(consumer.value);

    auto o = jv.get_object();
    int idv{-1};
    o["id"].to(idv);

    detail::executeOnSerializationFor(
        (ClientToSerializationMessagesIds)idv, jv, e, mc,
        std::make_index_sequence<(
            size_t)ClientToSerializationMessagesIds::num_clientToSerializationMessages>());
}

template <typename Client>
inline void clientThreadExecuteSerializationMessage(const std::string &msgView, Client *c)
{
    using namespace tao::json;

    // We need to unpack with the client message traits
    events::transformer<events::to_basic_value<detail::client_message_traits>> consumer;
    encoder::events::from_string(consumer, msgView);
    auto jv = std::move(consumer.value);

    auto o = jv.get_object();
    int idv{-1};
    o["id"].to(idv);

    detail::executeOnClientFor(
        (SerializationToClientMessageIds)idv, jv, c,
        std::make_index_sequence<(
            size_t)SerializationToClientMessageIds::num_serializationToClientMessages>());
}

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_CLIENT_SERIAL_IMPL_H
