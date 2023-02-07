//
// Created by Paul Walker on 2/5/23.
//

#ifndef SCXT_SRC_MESSAGING_CLIENT_DETAIL_CLIENT_SERIAL_IMPL_H
#define SCXT_SRC_MESSAGING_CLIENT_DETAIL_CLIENT_SERIAL_IMPL_H

#include "tao/json/to_string.hpp"
#include "tao/json/from_string.hpp"
#include "messaging/client/detail/client_json_details.h"

// This is a 'details only' file which you can safely ignore
// once it works, basically.
namespace scxt::messaging::client
{
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
        auto iid = (int)T::id;
        if constexpr (T::hasPayload)
        {
            client_message_value vt = t.msg.payload;
            v = {{"id", iid}, {"object", vt}};
        }
        else
        {
            v = {{"id", iid}};
        }
    }
};

template <typename T> struct ResponseWrapper
{
    const T &msg;
    SerializationToClientMessageIds id;
    ResponseWrapper(const T &m, SerializationToClientMessageIds i) : msg(m), id(i) {}
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
void doExecOnSerialization(tao::json::basic_value<Traits> &o, const engine::Engine &e,
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

        if constexpr (handler_t::hasPayload)
        {
            typename handler_t::payload_t payload;
            client_message_value mv = o.get_object()["object"];
            mv.to(payload);
            handler_t::executeOnSerialization(payload, e, mc);
        }
        else
        {
            handler_t::executeOnSerialization(e, mc);
        }
    }
}

template <template <typename...> class Traits, size_t... Is>
auto executeOnSerializationFor(size_t ft, typename tao::json::basic_value<Traits> &o,
                               const engine::Engine &e, MessageController &mc,
                               std::index_sequence<Is...>)
{
    using vtOp_t =
        void (*)(tao::json::basic_value<Traits> &, const engine::Engine &, MessageController &);
    constexpr vtOp_t fnc[] = {detail::doExecOnSerialization<Is>...};
    return fnc[ft](o, e, mc);
}

template <size_t I, typename Client, template <typename...> class Traits>
void doExecOnClient(tao::json::basic_value<Traits> &o, Client *c)
{
    typedef
        typename SerializationToClientType<(SerializationToClientMessageIds)I, Client>::T handler_t;
    if constexpr (std::is_same<handler_t, unimpl_t>::value)
    {
        assert(false);
        return;
    }
    else
    {
        handler_t handler;
        typename handler_t::payload_t payload;

        client_message_value mv = o.get_object()["object"];
        mv.to(payload);
        handler_t::executeOnResponse(c, payload);
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
    auto mw = detail::MessageWrapper(msg);
    detail::client_message_value v = mw;
    auto res = tao::json::to_string(v);
    mc.sendRawFromClient(res);
}

template <typename T>
inline void serializationSendToClient(SerializationToClientMessageIds id, const T &msg,
                                      messaging::MessageController &mc)
{
    auto mw = detail::ResponseWrapper<T>(msg, id);
    detail::client_message_value v = mw;
    auto res = tao::json::to_string(v);
    mc.clientCallback(res);
}

inline void serializationThreadExecuteClientMessage(const std::string &msgView,
                                                    const engine::Engine &e, MessageController &mc)
{
    using namespace tao::json;

    // We need to unpack with the client message traits
    events::transformer<events::to_basic_value<detail::client_message_traits>> consumer;
    events::from_string(consumer, msgView);
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
    events::from_string(consumer, msgView);
    auto jv = std::move(consumer.value);

    auto o = jv.get_object();
    int idv{-1};
    o["id"].to(idv);

    detail::executeOnClientFor(
        (SerializationToClientMessageIds)idv, jv, c,
        std::make_index_sequence<(
            size_t)SerializationToClientMessageIds::num_seralizationToClientMessages>());
}

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_CLIENT_SERIAL_IMPL_H
