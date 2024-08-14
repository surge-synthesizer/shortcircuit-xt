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

#ifndef SCXT_SRC_MESSAGING_CLIENT_MACRO_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_MACRO_MESSAGES_H

#include "client_macros.h"
#include "engine/group.h"
#include "selection/selection_manager.h"
#include "messaging/client/detail/message_helpers.h"

namespace scxt::messaging::client
{
using macroFullState_t = std::tuple<int16_t, int16_t, engine::Macro>;
SERIAL_TO_CLIENT(UpdateMacroFullState, s2c_update_macro_full_state, macroFullState_t,
                 onMacroFullState);

using macroValue_t = std::tuple<int16_t, int16_t, float>;
SERIAL_TO_CLIENT(UpdateMacroValue, s2c_update_macro_value, macroValue_t, onMacroValue);

inline void updateMacroFullState(const macroFullState_t &t, const engine::Engine &engine,
                                 MessageController &cont)
{
    const auto &[p, i, m] = t;
    cont.scheduleAudioThreadCallback(
        [part = p, index = i, macro = m](auto &e) {
            // Set everything except the value
            auto v = e.getPatch()->getPart(part)->macros[index].value;
            engine::Macro macroCopy = macro;
            macroCopy.setValueConstrained(v);
            e.getPatch()->getPart(part)->macros[index] = macroCopy;
        },
        [](const auto &e) {
            const auto &lz = e.getSelectionManager()->currentLeadZone(e);
            if (lz.has_value())
            {
                auto &zone =
                    e.getPatch()->getPart(lz->part)->getGroup(lz->group)->getZone(lz->zone);
                serializationSendToClient(messaging::client::s2c_update_zone_matrix_metadata,
                                          voice::modulation::getVoiceMatrixMetadata(*zone),
                                          *(e.getMessageController()));
            }

            const auto &lg = e.getSelectionManager()->currentLeadGroup(e);
            if (lg.has_value())
            {
                auto &group = e.getPatch()->getPart(lg->part)->getGroup(lg->group);
                serializationSendToClient(messaging::client::s2c_update_group_matrix_metadata,
                                          modulation::getGroupMatrixMetadata(*group),
                                          *(e.getMessageController()));
            }

            messaging::audio::SerializationToAudio s2am;
            s2am.id = audio::s2a_param_refresh;
            e.getMessageController()->sendSerializationToAudio(s2am);
        });
}
CLIENT_TO_SERIAL(SetMacroFullState, c2s_set_macro_full_state, macroFullState_t,
                 updateMacroFullState(payload, engine, cont));

inline void updateMacroValue(const macroValue_t &t, const engine::Engine &engine,
                             MessageController &cont)
{
    const auto &[p, i, f] = t;
    cont.scheduleAudioThreadCallback(
        [part = p, index = i, value = f](auto &e) {
            // Set the value
            auto &macro = e.getPatch()->getPart(part)->macros[index];
            macro.setValueConstrained(value);
        },
        [part = p, index = i](auto &e) {
            // a separate perhaps dropped message to update plugins
            messaging::audio::SerializationToAudio s2am;
            auto &macro = e.getPatch()->getPart(part)->macros[index];
            s2am.id = audio::s2a_param_set_value;
            s2am.payloadType = audio::SerializationToAudio::FOUR_FOUR_MIX;
            s2am.payload.mix.u[0] = engine::Macro::partIndexToMacroID(part, index);
            s2am.payload.mix.f[0] = macro.getValue01();

            e.getMessageController()->sendSerializationToAudio(s2am);
        });
}
CLIENT_TO_SERIAL(SetMacroValue, c2s_set_macro_value, macroValue_t,
                 updateMacroValue(payload, engine, cont));

using macroBeginEndEdit_t = std::tuple<bool, int16_t, int16_t>; // begin=true, id, val
inline void doMacroBeginEndEdit(const macroBeginEndEdit_t &payload, const engine::Engine &e,
                                messaging::MessageController &cont)
{
    auto [doIt, part, index] = payload;
    messaging::audio::SerializationToAudio s2am;
    s2am.id = audio::s2a_param_beginendedit;
    s2am.payloadType = audio::SerializationToAudio::FOUR_FOUR_MIX;
    auto &macro = e.getPatch()->getPart(part)->macros[index];

    s2am.payload.mix.u[0] = doIt;
    s2am.payload.mix.u[1] = engine::Macro::partIndexToMacroID(part, index);
    s2am.payload.mix.f[0] = macro.getValue01();

    cont.sendSerializationToAudio(s2am);
}
CLIENT_TO_SERIAL(MacroBeginEndEdit, c2s_macro_begin_end_edit, macroBeginEndEdit_t,
                 doMacroBeginEndEdit(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUITXT_MACRO_MESSAGES_H
