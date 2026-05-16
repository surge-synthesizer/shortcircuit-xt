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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_DETAIL_CLIENT_SERIAL_IMPL_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_DETAIL_CLIENT_SERIAL_IMPL_H

#include "messaging/client/client_serial.h"

// Inter-process messages can pack as JSON (the streaming format we use)
// or as msgpack messages which are a lot smaller and faster. PROCESS_AS_JSON
// is really just useful for debugging where you actually want to see a message.
#define PROCESS_AS_JSON 0
#if PROCESS_AS_JSON
#include "tao/json/to_string.hpp"
#include "tao/json/from_string.hpp"
#endif
#include "tao/json/contrib/traits.hpp"

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

// Dispatcher helpers (doExecOnSerialization / doExecOnClient / executeOnSerializationFor /
// executeOnClientFor) and the routing entry points
// (serializationThreadExecuteClientMessage / clientThreadExecuteSerializationMessage) used to
// live here, but they brought a per-message-id template instantiation tree into every TU that
// just wanted senders. They now live in client_serial_dispatch.h, which is included only by
// the three TUs that actually dispatch (messaging.cpp, SCXTEditor.cpp, console_ui.cpp).

} // namespace detail

template <typename T>
inline void clientSendToSerialization(const T &msg, messaging::MessageController &mc)
{
    assert(mc.threadingChecker.isClientThread());
    auto mw = detail::MessageWrapper(msg);
    detail::client_message_value v = mw;
    auto res = encoder::to_string(v);

    mc.sendRawFromClient(res);
}

template <typename T>
inline void serializationSendToClient(SerializationToClientMessageIds id, const T &msg,
                                      messaging::MessageController &mc)
{
    assert(mc.threadingChecker.isSerialThread());

    try
    {
        auto lk = mc.acquireClientCallbackMutex();
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
            RAISE_ERROR_CONT(mc, "JSON Streaming Error", e.what());
        }
    }
}

// serializationThreadExecuteClientMessage is defined out-of-line in messaging.cpp.
// clientThreadExecuteSerializationMessage<Client> body lives in client_serial_dispatch.h and is
// explicitly instantiated in the client TUs (SCXTEditor.cpp, console_ui.cpp). The matching
// extern template declarations live in client_serial.h.

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_CLIENT_SERIAL_IMPL_H
