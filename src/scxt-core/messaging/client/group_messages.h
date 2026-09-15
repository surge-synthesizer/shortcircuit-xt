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

#ifndef SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_GROUP_MESSAGES_H
#define SCXT_SRC_SCXT_CORE_MESSAGING_CLIENT_GROUP_MESSAGES_H

#include "messaging/client/detail/client_serial_impl.h"
#include "client_macros.h"
#include "engine/group.h"
#include "selection/selection_manager.h"
#include "messaging/client/detail/message_helpers.h"
#include "undo_manager/payload_undoable_items.h"
#include "part_messages.h" // a trigger edit refreshes the part's keyswitch display

namespace scxt::messaging::client
{

using groupOutputInfoUpdate_t = std::pair<bool, engine::Group::GroupOutputInfo>;
SERIAL_TO_CLIENT(GroupOutputInfoUpdated, s2c_update_group_output_info, groupOutputInfoUpdate_t,
                 onGroupOutputInfoUpdated);

SERIAL_TO_CLIENT(SendGroupTriggerConditions, s2c_send_group_trigger_conditions,
                 scxt::engine::GroupTriggerConditions, onGroupTriggerConditions)

CLIENT_TO_SERIAL_CONSTRAINED(UpdateGroupOutputFloatValue, c2s_update_group_output_float_value,
                             detail::diffMsg_t<float>, engine::Group::GroupOutputInfo,
                             detail::updateGroupMemberValue<undo::GroupOutputInfoSpec>(
                                 &engine::Group::outputInfo, payload, engine, cont));

CLIENT_TO_SERIAL_CONSTRAINED(UpdateGroupOutputInt16TValue, c2s_update_group_output_int16_t_value,
                             detail::diffMsg_t<int16_t>, engine::Group::GroupOutputInfo,
                             detail::updateGroupMemberValue<undo::GroupOutputInfoSpec>(
                                 &engine::Group::outputInfo, payload, engine, cont));

CLIENT_TO_SERIAL_CONSTRAINED(UpdateGroupOutputBoolValue, c2s_update_group_output_bool_value,
                             detail::diffMsg_t<bool>, engine::Group::GroupOutputInfo,
                             detail::updateGroupMemberValue<undo::GroupOutputInfoSpec>(
                                 &engine::Group::outputInfo, payload, engine, cont));

inline void doUpdateGroupTriggerConditions(const engine::GroupTriggerConditions &payload,
                                           engine::Engine &engine, MessageController &cont)
{
    auto ga = engine.getSelectionManager()->currentLeadGroup(engine);
    if (ga.has_value())
    {
        undo::pushPayloadUndoFor<undo::GroupTriggerConditionsSpec>(engine, {*ga});
        cont.scheduleAudioThreadCallback(
            [p = payload, g = *ga](auto &eng) {
                auto &grp = eng.getPatch()->getPart(g.part)->getGroup(g.group);
                grp->triggerConditions = p;
                grp->triggerConditions.setupOnUnstream(
                    eng.getPatch()->getPart(g.part)->groupTriggerInstrumentState);
                eng.getPatch()->getPart(g.part)->guaranteeKeyswitchLatchCoherence(eng);
            },
            [g = *ga](const auto &eng) {
                // the edit may have moved a switch key or the live articulation
                eng.sendKeySwitchStateToClient((int16_t)g.part);
            });
    }
}

CLIENT_TO_SERIAL(UpdateGroupTriggerConditions, c2s_update_group_trigger_conditions,
                 scxt::engine::GroupTriggerConditions,
                 doUpdateGroupTriggerConditions(payload, engine, cont));

inline void doUpdateGroupOutputInfoPolyphony(const scxt::engine::Group::GroupOutputInfo payload,
                                             engine::Engine &engine,
                                             messaging::MessageController &cont)
{
    auto ga = engine.getSelectionManager()->currentLeadGroup(engine);
    if (ga.has_value())
    {
        undo::pushPayloadUndoFor<undo::GroupOutputInfoSpec>(engine, {*ga});
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

inline void doUpdateGroupOutputInfoMidiChannel(const scxt::engine::Group::GroupOutputInfo payload,
                                               engine::Engine &engine,
                                               messaging::MessageController &cont)
{
    auto ga = engine.getSelectionManager()->currentLeadGroup(engine);
    if (ga.has_value())
    {
        undo::pushPayloadUndoFor<undo::GroupOutputInfoSpec>(engine, {*ga});
        cont.scheduleAudioThreadCallback([p = payload, g = *ga](auto &eng) {
            auto &grp = eng.getPatch()->getPart(g.part)->getGroup(g.group);
            grp->outputInfo = p;
            grp->onGroupMidiChannelSubscriptionChanged();
        });
    }
}
CLIENT_TO_SERIAL(UpdateGroupOutputInfoMidiChannel, c2s_update_group_output_info_midichannel,
                 scxt::engine::Group::GroupOutputInfo,
                 doUpdateGroupOutputInfoMidiChannel(payload, engine, cont));

inline void
doUpdateGroupOutputInfoExclusiveGroup(const scxt::engine::Group::GroupOutputInfo payload,
                                      engine::Engine &engine, messaging::MessageController &cont)
{
    auto ga = engine.getSelectionManager()->currentLeadGroup(engine);
    if (ga.has_value())
    {
        undo::pushPayloadUndoFor<undo::GroupOutputInfoSpec>(engine, {*ga});
        cont.scheduleAudioThreadCallback([p = payload, g = *ga](auto &eng) {
            auto &grp = eng.getPatch()->getPart(g.part)->getGroup(g.group);
            grp->outputInfo = p;
        });
    }
}
CLIENT_TO_SERIAL(UpdateGroupOutputInfoExclusiveGroup, c2s_update_group_output_info_exclusive_group,
                 scxt::engine::Group::GroupOutputInfo,
                 doUpdateGroupOutputInfoExclusiveGroup(payload, engine, cont));

enum MuteOrSoloGesture : int32_t
{
    MS_THIS_GROUP = 0,
    MS_SELECTED_GROUPS, // every selected group, if this one is selected
    MS_EXCLUSIVE,       // this group alone, or clear all if it already was
    MS_RANGE            // sweep to the nearest group already in the new state
};

using muteOrSoloGroup_t =
    std::tuple<int32_t, int32_t, bool, bool, int32_t>; // part, group, isSolo, value, gesture
inline void doMuteOrSoloGroup(const muteOrSoloGroup_t &payload, engine::Engine &engine,
                              messaging::MessageController &cont)
{
    auto &[p, g, isSolo, value, gesture] = payload;
    if (p < 0 || p >= numParts)
        return;
    const auto &part = engine.getPatch()->getPart(p);
    auto ng = (int32_t)part->getGroups().size();
    if (g < 0 || g >= ng)
        return;

    auto stateOf = [&part, isSolo](int32_t i) {
        const auto &oi = part->getGroup(i)->outputInfo;
        return isSolo ? oi.soloed : oi.muted;
    };

    std::vector<bool> target(ng);
    for (int32_t i = 0; i < ng; ++i)
        target[i] = stateOf(i);

    switch ((MuteOrSoloGesture)gesture)
    {
    case MS_SELECTED_GROUPS:
    {
        const auto &gs = engine.getSelectionManager()->state[p].selectedGroups;
        bool inSelection = std::any_of(
            gs.begin(), gs.end(), [p, g](const auto &gi) { return gi.part == p && gi.group == g; });
        target[g] = value;
        if (inSelection)
        {
            for (const auto &gi : gs)
                if (gi.part == p && gi.group >= 0 && gi.group < ng)
                    target[gi.group] = value;
        }
    }
    break;
    case MS_EXCLUSIVE:
    {
        bool onlyThisOne = stateOf(g);
        for (int32_t i = 0; i < ng; ++i)
            if (i != g && stateOf(i))
                onlyThisOne = false;
        for (int32_t i = 0; i < ng; ++i)
            target[i] = !onlyThisOne && i == g;
    }
    break;
    case MS_RANGE:
    {
        auto anchor = g;
        for (int32_t d = 1; d < ng && anchor == g; ++d)
        {
            if (g - d >= 0 && stateOf(g - d) == value)
                anchor = g - d;
            else if (g + d < ng && stateOf(g + d) == value)
                anchor = g + d;
        }
        for (int32_t i = std::min(g, anchor); i <= std::max(g, anchor); ++i)
            target[i] = value;
    }
    break;
    default:
        target[g] = value;
        break;
    }

    std::vector<selection::SelectionManager::ZoneAddress> changed;
    std::vector<std::pair<int32_t, bool>> newValues;
    for (int32_t i = 0; i < ng; ++i)
    {
        if (target[i] != stateOf(i))
        {
            changed.emplace_back(p, i, -1);
            newValues.emplace_back(i, target[i]);
        }
    }

    auto sendStructure = [p](const engine::Engine &e) {
        serializationSendToClient(s2c_send_pgz_structure, e.getPartGroupZoneStructure(),
                                  *e.getMessageController());
        // keep the client's lead group output info current, since some edits send it back whole
        auto lg = e.getSelectionManager()->currentLeadGroup(e);
        if (lg.has_value() && lg->part == p)
        {
            const auto &grp = e.getPatch()->getPart(p)->getGroup(lg->group);
            serializationSendToClient(s2c_update_group_output_info,
                                      groupOutputInfoUpdate_t{true, grp->outputInfo},
                                      *e.getMessageController());
        }
    };

    // the toggle flipped itself on the client, so a no-op still owes it the real state
    if (changed.empty())
    {
        sendStructure(engine);
        return;
    }

    undo::pushPayloadUndoFor<undo::GroupOutputInfoSpec>(engine, changed);

    cont.scheduleAudioThreadCallback(
        [p, isSolo, newValues](auto &eng) {
            const auto &prt = eng.getPatch()->getPart(p);
            for (const auto &[gi, v] : newValues)
            {
                if (gi >= (int32_t)prt->getGroups().size())
                    continue;
                auto &oi = prt->getGroup(gi)->outputInfo;
                if (isSolo)
                    oi.soloed = v;
                else
                    oi.muted = v;
            }
        },
        sendStructure);
}
CLIENT_TO_SERIAL(MuteOrSoloGroup, c2s_mute_solo_group, muteOrSoloGroup_t,
                 doMuteOrSoloGroup(payload, engine, cont));

} // namespace scxt::messaging::client
#endif // SHORTCIRCUIT_GROUP_MESSAGES_H
