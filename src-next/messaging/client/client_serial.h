//
// Created by Paul Walker on 2/5/23.
//

#ifndef SCXT_SRC_MESSAGING_CLIENT_CLIENT_SERIAL_H
#define SCXT_SRC_MESSAGING_CLIENT_CLIENT_SERIAL_H

#include "messaging/messaging.h"

namespace scxt::messaging::client
{
/**
 * These IDs are used inside a session only and are not streamed,
 * so add whatever you want as long as (1) you keep them contig
 * (so don't assign values) and (2) the num_ enum is the last one
 */
enum ClientToSerializationMessagesIds
{
    c2s_refresh_patch,

    num_clientToSerializationMessages
};

enum SerializationToClientMessageIds
{
    s2c_patch_stream,
    s2c_voice_count,
    
    num_seralizationToClientMessages
};

template <typename T> void clientSendMessage(const T &message, MessageController &mc);
template <typename T> void serializationSendToClient(const T &message, MessageController &mc);

typedef uint8_t unimpl_t;
template <ClientToSerializationMessagesIds id> struct ClientToSerializationType
{
    typedef unimpl_t T;
};

template <SerializationToClientMessageIds id, typename Client> struct SerializationClientType
{
    typedef unimpl_t T;
};

void serializationThreadExecuteClientMessage(const std::string &msgView, const engine::Engine &e,
                                             MessageController &mc);

template <typename T>
void serializationSendToClient(SerializationToClientMessageIds id, const T &payload,
                               messaging::MessageController &mc);

template <typename Client>
void clientThreadExecuteSerializationMessage(const std::string &msgView, Client *c);

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_CLIENT_SERIAL_H
