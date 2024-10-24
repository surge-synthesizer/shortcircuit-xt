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

#ifndef SCXT_SRC_MESSAGING_CLIENT_GROUP_MESSAGES_H
#define SCXT_SRC_MESSAGING_CLIENT_GROUP_MESSAGES_H

#include "client_macros.h"
#include "engine/group.h"
#include "selection/selection_manager.h"
#include "messaging/client/detail/message_helpers.h"

namespace scxt::messaging::client
{

using groupOutputInfoUpdate_t = std::pair<bool, engine::Group::GroupOutputInfo>;
SERIAL_TO_CLIENT(GroupOutputInfoUpdated, s2c_update_group_output_info, groupOutputInfoUpdate_t,
                 onGroupOutputInfoUpdated);

SERIAL_TO_CLIENT(SendGroupTriggerConditions, s2c_send_group_trigger_conditions,
                 scxt::engine::GroupTriggerConditions, onGroupTriggerConditions)

CLIENT_TO_SERIAL_CONSTRAINED(UpdateGroupOutputFloatValue, c2s_update_group_output_float_value,
                             detail::diffMsg_t<float>, engine::Group::GroupOutputInfo,
                             detail::updateGroupMemberValue(&engine::Group::outputInfo, payload,
                                                            engine, cont));

CLIENT_TO_SERIAL_CONSTRAINED(UpdateGroupOutputInt16TValue, c2s_update_group_output_int16_t_value,
                             detail::diffMsg_t<int16_t>, engine::Group::GroupOutputInfo,
                             detail::updateGroupMemberValue(&engine::Group::outputInfo, payload,
                                                            engine, cont));

CLIENT_TO_SERIAL_CONSTRAINED(UpdateGroupOutputBoolValue, c2s_update_group_output_bool_value,
                             detail::diffMsg_t<bool>, engine::Group::GroupOutputInfo,
                             detail::updateGroupMemberValue(&engine::Group::outputInfo, payload,
                                                            engine, cont));

inline void doUpdateGroupTriggerConditions(const engine::GroupTriggerConditions &payload,
                                           const engine::Engine &engine, MessageController &cont)
{
    auto ga = engine.getSelectionManager()->currentLeadGroup(engine);
    if (ga.has_value())
    {
        cont.scheduleAudioThreadCallback([p = payload, g = *ga](auto &eng) {
            auto &grp = eng.getPatch()->getPart(g.part)->getGroup(g.group);
            grp->triggerConditions = p;
            grp->triggerConditions.setupOnUnstream(
                eng.getPatch()->getPart(g.part)->groupTriggerInstrumentState);
        });
    }
}

CLIENT_TO_SERIAL(UpdateGroupTriggerConditions, c2s_update_group_trigger_conditions,
                 scxt::engine::GroupTriggerConditions,
                 doUpdateGroupTriggerConditions(payload, engine, cont));

inline void doUpdateGroupOutputInfoPolyphony(const scxt::engine::Group::GroupOutputInfo payload,
                                             const engine::Engine &engine,
                                             messaging::MessageController &cont)
{
    auto ga = engine.getSelectionManager()->currentLeadGroup(engine);
    if (ga.has_value())
    {
        cont.scheduleAudioThreadCallback([p = payload, g = *ga](auto &eng) {
            auto &grp = eng.getPatch()->getPart(g.part)->getGroup(g.group);
            grp->outputInfo = p;
            grp->resetPolyAndPlaymode(eng);
        });
    }
}
CLIENT_TO_SERIAL(UpdateGroupOutputInfoPolyphony, c2s_update_group_output_info_polyphony,
                 scxt::engine::Group::GroupOutputInfo,
                 doUpdateGroupOutputInfoPolyphony(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_GROUP_MESSAGES_H
