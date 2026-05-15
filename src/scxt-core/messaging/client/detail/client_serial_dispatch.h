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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_DETAIL_CLIENT_SERIAL_DISPATCH_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_DETAIL_CLIENT_SERIAL_DISPATCH_H

// Dispatcher infrastructure for the message router. This header is intentionally
// included by only the three TUs that actually dispatch messages:
//   - messaging.cpp                       (c2s, calls executeOnSerializationFor)
//   - app/editor-impl/SCXTEditor.cpp      (s2c → SCXTEditorReceiver)
//   - clients/console-ui/console_ui.cpp   (s2c → ConsoleUI)
//
// Keeping the helpers (doExecOnSerialization/doExecOnClient/...) out of
// client_serial_impl.h means TUs that only send messages don't instantiate
// the per-message-id helper templates (which were a multi-thousand-instantiation
// hot spot in the build trace).
//
// `clientThreadExecuteSerializationMessage<Client>` is explicitly instantiated
// in the two client TUs above; client_serial.h carries the matching extern
// template declarations so a future TU that #includes a client header without
// the dispatch header still gets a clean link error rather than silent extra
// instantiations.

#include "messaging/client/detail/client_serial_impl.h"

namespace scxt::messaging::client
{
namespace detail
{
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

template <typename Client>
void clientThreadExecuteSerializationMessage(const std::string &msgView, Client *c)
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
#endif // SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_DETAIL_CLIENT_SERIAL_DISPATCH_H
