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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_MACRO_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_MACRO_MESSAGES_H

#include "client_macros.h"
#include "engine/group.h"
#include "selection/selection_manager.h"
#include "messaging/client/detail/message_helpers.h"
#include "undo_manager/macro_undoable_items.h"

namespace scxt::messaging::client
{
using macroFullState_t = std::tuple<int16_t, int16_t, engine::Macro>;
SERIAL_TO_CLIENT(UpdateMacroFullState, s2c_update_macro_full_state, macroFullState_t,
                 onMacroFullState);

using macroValue_t = std::tuple<int16_t, int16_t, float>;
SERIAL_TO_CLIENT(UpdateMacroValue, s2c_update_macro_value, macroValue_t, onMacroValue);

inline void updateMacroFullStateApply(int16_t part, int16_t index, const engine::Macro &macroR,
                                      bool fromUndo, MessageController &cont)
{
    cont.scheduleAudioThreadCallback(
        [part, index, macro = macroR, fromUndo](auto &e) {
            engine::Macro macroCopy = macro;
            if (!fromUndo)
            {
                // Set everything except the value except when undoing (to avoid colliding with
                // automation)
                auto v = e.getPatch()->getPart(part)->macros[index].value;
                macroCopy.setValueConstrained(v);
            }
            e.getPatch()->getPart(part)->macros[index] = macroCopy;
        },
        [fromUndo, part, index](const auto &e) {
            if (fromUndo)
            {
                auto &macro = e.getPatch()->getPart(part)->macros[index];
                serializationSendToClient(messaging::client::s2c_update_macro_full_state,
                                          macroFullState_t{part, index, macro},
                                          *(e.getMessageController()));
                serializationSendToClient(messaging::client::s2c_update_macro_value,
                                          macroValue_t{part, index, macro.value},
                                          *(e.getMessageController()));
            }

            const auto &lz = e.getSelectionManager()->currentLeadZone(e);
            if (lz.has_value())
            {
                auto &zone =
                    e.getPatch()->getPart(lz->part)->getGroup(lz->group)->getZone(lz->zone);
                if (zone)
                {
                    serializationSendToClient(messaging::client::s2c_update_zone_matrix_metadata,
                                              voice::modulation::getVoiceMatrixMetadata(*zone),
                                              *(e.getMessageController()));
                }
            }

            const auto &lg = e.getSelectionManager()->currentLeadGroup(e);
            if (lg.has_value())
            {
                auto &group = e.getPatch()->getPart(lg->part)->getGroup(lg->group);
                if (group)
                {
                    serializationSendToClient(messaging::client::s2c_update_group_matrix_metadata,
                                              modulation::getGroupMatrixMetadata(*group),
                                              *(e.getMessageController()));
                }
            }

            messaging::audio::SerializationToAudio s2am;
            s2am.id = audio::s2a_param_refresh;
            e.getMessageController()->sendSerializationToAudio(s2am);
        });
}

inline void updateMacroFullState(const macroFullState_t &t, engine::Engine &engine,
                                 MessageController &cont)
{
    const auto &[p, i, m] = t;
    if (p < 0 || p >= numParts || i < 0 || i >= macrosPerPart)
    {
        SCLOG_IF(warnings, "Invalid part/index in updateMacroFullState: " << p << "/" << i);
        return;
    }

    {
        auto undoItem = std::make_unique<undo::MacroFullStateItem>();
        undoItem->store(engine, p, i);
        engine.undoManager.storeUndoStep(std::move(undoItem));
    }

    updateMacroFullStateApply(p, i, m, false, cont);
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
            auto &partO = e.getPatch()->getPart(part); // ->macros[index];
            partO->macroLagHandler.setTargetOnMacro(index, value);
            if (!partO->isActive())
                partO->macroLagHandler.instantlySnap();
            // macro.setValueConstrained(value);
        },
        [part = p, index = i, value = f](auto &e) {
            // a separate perhaps dropped message to update plugins
            messaging::audio::SerializationToAudio s2am;
            auto &macro = e.getPatch()->getPart(part)->macros[index];
            s2am.id = audio::s2a_param_set_value;
            s2am.payloadType = audio::SerializationToAudio::FOUR_FOUR_MIX;
            s2am.payload.mix.u[0] = engine::Macro::partIndexToMacroID(part, index);
            s2am.payload.mix.f[0] = macro.getValue01For(value);

            e.getMessageController()->sendSerializationToAudio(s2am);
        });
}
CLIENT_TO_SERIAL(SetMacroValue, c2s_set_macro_value, macroValue_t,
                 updateMacroValue(payload, engine, cont));

using macroBeginEndEdit_t = std::tuple<bool, int16_t, int16_t>; // begin=true, id, val
inline void doMacroBeginEndEdit(const macroBeginEndEdit_t &payload, engine::Engine &e,
                                messaging::MessageController &cont)
{
    auto [doIt, part, index] = payload;

    if (doIt)
    {
        auto undoItem = std::make_unique<undo::MacroFullStateItem>();
        undoItem->store(e, part, index);
        e.undoManager.storeUndoStep(std::move(undoItem));
    }

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
