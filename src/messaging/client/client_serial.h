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

#ifndef SCXT_SRC_MESSAGING_CLIENT_CLIENT_SERIAL_H
#define SCXT_SRC_MESSAGING_CLIENT_CLIENT_SERIAL_H

#include "messaging/messaging.h"

namespace scxt::messaging::client
{
/**
 * These IDs are used inside a session only and are not streamed,
 * so add whatever you want as long as (1) you keep them contig
 * (so don't assign values) and (2) the num_ enum is the last one
 *
 * These ids follow a naming convention which matches with the objects
 * created in the CLIENT_TO_SERIAL.. and SERIAL_TO_CLIENT... macros
 * used throughout messaging/client. That naming convention is
 *
 * S2C:
 * - Prefer the verb "Send", since S2C is sending authoritative state
 * - Use the structure s2c_send_class_thing, for instance
 *   s2c_send_selection_state
 * - Name the object a camelcase version of the enum without s2c,
 *   so for instance SendSelectionState
 * - Make the SCXT Editor callback names onSelectionState
 * - name the payloads as objectNamePayload_t so selectionStatePayload_t
 *   if the payload type is a custom type.
 *
 * C2S:
 * - Prefer the verb corresponding to the action like set, swap, create
 *   delete, etc. Do not use send in a C2S. Prefer set over update.
 * - Class name follows per S2C above as does payload name
 * - If an inline function is used to handle a message use the name
 *   'doObjectName' so 'doSetZoneOrGroupModstorage`
 */
enum ClientToSerializationMessagesIds
{
    // Registration and Reset Messages
    c2s_register_client,
    c2s_reset_engine,

    // Stream and IO Messages
    c2s_unstream_engine_state,
    c2s_save_multi,
    c2s_save_part,

    c2s_load_multi,
    c2s_load_part_into,

    c2s_apply_select_actions,
    c2s_select_part,

    c2s_begin_edit, // not implemented yet
    c2s_end_edit,   // implemented as a hammer

    c2s_update_zone_or_group_eg_float_value,
    c2s_update_zone_or_group_modstorage_float_value,
    c2s_update_zone_or_group_modstorage_bool_value,
    c2s_update_zone_or_group_modstorage_int16_t_value,

    c2s_update_full_modstorage_for_groups_or_zones,
    c2s_update_miscmod_storage_for_groups_or_zones,

    c2s_update_lead_zone_mapping,
    c2s_update_zone_mapping_float,
    c2s_update_zone_mapping_int16_t,
    c2s_update_lead_zone_single_variant,
    c2s_update_zone_variants_int16_t,

    c2s_normalize_variant_amplitude,
    c2s_clear_variant_amplitude_normalization,

    c2s_delete_variant,

    c2s_update_zone_output_float_value,
    c2s_update_zone_output_int16_t_value,
    c2s_update_zone_output_int16_t_value_then_refresh,

    c2s_update_zone_routing_row,

    c2s_update_group_routing_row,
    c2s_update_group_output_float_value,
    c2s_update_group_output_int16_t_value,
    c2s_update_group_output_bool_value,
    c2s_update_group_output_info_polyphony,
    c2s_update_group_output_info_midichannel,

    c2s_update_group_trigger_conditions,

    c2s_request_zone_data_refresh,

    // #1141 done up until here. Below this point the name rubric above isn't confirmed in place
    c2s_request_pgz_structure, // ?

    c2s_update_single_processor_float_value,
    c2s_update_single_processor_bool_value,
    c2s_update_single_processor_int32_t_value,
    c2s_set_processor_type,
    c2s_copy_processor_lead_to_all,
    c2s_swap_zone_processors,
    c2s_swap_group_processors,

    c2s_add_sample,
    c2s_add_sample_with_range,
    c2s_add_sample_in_zone,
    c2s_add_compound_element_with_range,
    c2s_add_compound_element_in_zone,
    c2s_create_group,
    c2s_move_zone,
    c2s_delete_zone,
    c2s_duplicate_zone,
    c2s_copy_zone,
    c2s_paste_zone,
    c2s_add_blank_zone,
    c2s_delete_selected_zones,
    c2s_delete_group,
    c2s_move_group,
    c2s_clear_part,
    c2s_rename_zone,
    c2s_rename_group,

    c2s_set_tuning_mode,

    c2s_noteonoff,

    c2s_initialize_mixer,
    c2s_set_mixer_effect,
    c2s_set_mixer_effect_storage,
    c2s_set_mixer_send_storage,

    // Browser functions
    c2s_add_browser_device_location,
    c2s_reindex_browser_location,
    c2s_request_browser_update, // client asks us to push browser info
    c2s_remove_browser_device_location,
    c2s_preview_browser_sample,
    c2s_browser_queue_refresh, // this is a "special" message see browser_messages

    c2s_request_debug_action,
    c2s_raise_debug_error,

    c2s_silence_engine,

    c2s_set_macro_full_state,
    c2s_set_macro_value,

    c2s_request_host_callback,
    c2s_macro_begin_end_edit,

    c2s_set_othertab_selection,

    c2s_send_full_part_config,

    c2s_resolve_sample,

    // part activation
    c2s_activate_next_part,
    c2s_deactivate_part,

    num_clientToSerializationMessages
};

enum SerializationToClientMessageIds
{
    s2c_report_error,
    s2c_send_initial_metadata,
    s2c_send_debug_info,
    s2c_send_activity_notification,

    s2c_engine_status,
    s2c_update_group_or_zone_adsr_view,
    s2c_respond_zone_mapping,
    s2c_respond_zone_samples,
    s2c_send_pgz_structure,
    s2c_send_all_processor_descriptions,
    s2c_update_zone_matrix_metadata,
    s2c_update_zone_matrix,
    s2c_update_group_matrix_metadata,
    s2c_update_group_matrix,
    s2c_update_group_or_zone_individual_modulator_storage,
    s2c_update_group_or_zone_miscmod_storage,
    s2c_update_zone_output_info,
    s2c_update_group_output_info,

    s2c_send_clipboard_type,

    s2c_send_group_trigger_conditions,

    s2c_respond_single_processor_metadata_and_data,

    s2c_send_part_configuration,
    s2c_send_selected_part,
    s2c_send_selected_group_zone_mapping_summary,
    s2c_send_selection_state,
    s2c_send_othertab_selection,

    s2c_bus_effect_full_data,
    s2c_bus_send_data,

    s2c_refresh_browser,
    s2c_send_browser_queuelength,

    s2c_update_macro_full_state,
    s2c_update_macro_value,

    s2c_send_missing_resolution_workitem_list,

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
