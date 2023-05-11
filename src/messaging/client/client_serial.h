/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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
    c2s_on_register,

    c2s_single_select_address,
    c2s_do_select_action,
    c2s_do_multi_select_action,

    c2s_update_zone_adsr_view,

    c2s_update_zone_mapping,
    c2s_update_zone_samples,
    c2s_update_zone_routing_row,

    c2s_update_individual_lfo,

    c2s_request_pgz_structure,

    c2s_update_single_processor_data,
    c2s_set_processor_type,

    c2s_add_sample,

    c2s_set_tuning_mode,

    c2s_noteonoff,

    c2s_initialize_mixer,
    c2s_set_mixer_effect,
    c2s_set_mixer_effect_storage,

    num_clientToSerializationMessages
};

enum SerializationToClientMessageIds
{
    s2c_report_error,
    s2c_send_initial_metadata,
    s2c_engine_status,
    s2c_respond_zone_adsr_view,
    s2c_respond_zone_mapping,
    s2c_respond_zone_samples,
    s2c_send_pgz_structure,
    s2c_send_all_processor_descriptions,
    s2c_update_zone_voice_matrix_metadata,
    s2c_update_zone_voice_matrix,
    s2c_update_zone_individual_lfo,

    s2c_respond_single_processor_metadata_and_data,

    s2c_send_selected_group_zone_mapping_summary,
    s2c_send_selection_state,

    s2c_initialize_mixer,
    s2c_bus_effect_full_data,

    num_serializationToClientMessages
};

inline bool cacheSerializationMessagesAbsentClient(SerializationToClientMessageIds id)
{
    switch (id)
    {
    case s2c_report_error:
        return true;
    default:
        break;
    }
    return false;
}

typedef uint8_t unimpl_t;
template <ClientToSerializationMessagesIds id> struct ClientToSerializationType
{
    typedef unimpl_t T;
};

template <SerializationToClientMessageIds id> struct SerializationToClientType
{
    typedef unimpl_t T;
};

template <typename T> void clientSendToSerialization(const T &message, MessageController &mc);
template <typename T>
void serializationSendToClient(SerializationToClientMessageIds id, const T &payload,
                               messaging::MessageController &mc);

void serializationThreadExecuteClientMessage(const std::string &msgView, engine::Engine &e,
                                             MessageController &mc);
template <typename Client>
void clientThreadExecuteSerializationMessage(const std::string &msgView, Client *c);

} // namespace scxt::messaging::client

#endif // SHORTCIRCUIT_CLIENT_SERIAL_H
