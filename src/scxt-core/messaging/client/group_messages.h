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

/*
 * Every output info edit is a field named by its offset, so it reaches the whole group selection
 * and pays whatever side effect that field owes. The menus in the group settings pane send these
 * too; a whole-struct write would carry the lead's gain and pan onto every selected group.
 */
CLIENT_TO_SERIAL_CONSTRAINED(UpdateGroupOutputFloatValue, c2s_update_group_output_float_value,
                             detail::diffMsg_t<float>, engine::Group::GroupOutputInfo,
                             detail::updateGroupMemberValue<undo::GroupOutputInfoSpec>(
                                 &engine::Group::outputInfo, payload, engine, cont, nullptr,
                                 &engine::Group::outputInfoFieldEdited));

CLIENT_TO_SERIAL_CONSTRAINED(UpdateGroupOutputInt16TValue, c2s_update_group_output_int16_t_value,
                             detail::diffMsg_t<int16_t>, engine::Group::GroupOutputInfo,
                             detail::updateGroupMemberValue<undo::GroupOutputInfoSpec>(
                                 &engine::Group::outputInfo, payload, engine, cont, nullptr,
                                 &engine::Group::outputInfoFieldEdited));

CLIENT_TO_SERIAL_CONSTRAINED(UpdateGroupOutputInt32TValue, c2s_update_group_output_int32_t_value,
                             detail::diffMsg_t<int32_t>, engine::Group::GroupOutputInfo,
                             detail::updateGroupMemberValue<undo::GroupOutputInfoSpec>(
                                 &engine::Group::outputInfo, payload, engine, cont, nullptr,
                                 &engine::Group::outputInfoFieldEdited));

CLIENT_TO_SERIAL_CONSTRAINED(UpdateGroupOutputBoolValue, c2s_update_group_output_bool_value,
                             detail::diffMsg_t<bool>, engine::Group::GroupOutputInfo,
                             detail::updateGroupMemberValue<undo::GroupOutputInfoSpec>(
                                 &engine::Group::outputInfo, payload, engine, cont, nullptr,
                                 &engine::Group::outputInfoFieldEdited));

/*
 * Which parts of a trigger payload the user actually touched. The client edits a copy of the
 * lead's conditions and posts the whole struct back, so the edit is whatever differs from the
 * lead as the engine still holds it, and only that travels to the rest of the selection. A row
 * whose type changed moves as a row, because its args mean something else once its id does.
 */
struct groupTriggerDelta_t
{
    std::array<bool, scxt::triggerConditionsPerGroup> row{}, active{};
    std::array<std::array<bool, engine::GroupTriggerStorage::numArgs>,
               scxt::triggerConditionsPerGroup>
        arg{};
    std::array<bool, scxt::triggerConditionsPerGroup - 1> conjunction{};
    bool voiceCreationMode{false}, releaseCountdown{false}, releaseOnePerKey{false};
};

inline groupTriggerDelta_t diffGroupTriggers(const engine::GroupTriggerConditions &was,
                                             const engine::GroupTriggerConditions &now)
{
    groupTriggerDelta_t d;
    d.voiceCreationMode = was.voiceCreationMode != now.voiceCreationMode;
    d.releaseCountdown = was.releaseCountdownSeconds != now.releaseCountdownSeconds;
    d.releaseOnePerKey = was.releaseOnePerKey != now.releaseOnePerKey;
    for (int i = 0; i < scxt::triggerConditionsPerGroup; ++i)
    {
        d.row[i] = was.storage[i].id != now.storage[i].id;
        d.active[i] = was.active[i] != now.active[i];
        for (int j = 0; j < engine::GroupTriggerStorage::numArgs; ++j)
            d.arg[i][j] = was.storage[i].args[j] != now.storage[i].args[j];
    }
    for (int i = 0; i < scxt::triggerConditionsPerGroup - 1; ++i)
        d.conjunction[i] = was.conjunctions[i] != now.conjunctions[i];
    return d;
}

inline void applyGroupTriggerDelta(engine::GroupTriggerConditions &tc,
                                   const engine::GroupTriggerConditions &p,
                                   const groupTriggerDelta_t &d)
{
    for (int i = 0; i < scxt::triggerConditionsPerGroup; ++i)
    {
        // arg 1 of a round robin is the group's ordinal, which is per-group by definition;
        // setupOnUnstream clamps whatever this leaves behind back into an ordinal
        auto ordinalIsTheirs = engine::isRoundRobinTriggerID(p.storage[i].id);

        if (d.row[i])
        {
            tc.storage[i].id = p.storage[i].id;
            for (int j = 0; j < engine::GroupTriggerStorage::numArgs; ++j)
                if (!(j == 1 && ordinalIsTheirs))
                    tc.storage[i].args[j] = p.storage[i].args[j];
        }
        else if (tc.storage[i].id == p.storage[i].id)
        {
            // an arg means something only on a row of the same trigger type
            for (int j = 0; j < engine::GroupTriggerStorage::numArgs; ++j)
                if (d.arg[i][j] && !(j == 1 && ordinalIsTheirs))
                    tc.storage[i].args[j] = p.storage[i].args[j];
        }
        if (d.active[i])
            tc.active[i] = p.active[i];
    }
    for (int i = 0; i < scxt::triggerConditionsPerGroup - 1; ++i)
        if (d.conjunction[i])
            tc.conjunctions[i] = p.conjunctions[i];
    if (d.voiceCreationMode)
        tc.voiceCreationMode = p.voiceCreationMode;
    if (d.releaseCountdown)
        tc.releaseCountdownSeconds = p.releaseCountdownSeconds;
    if (d.releaseOnePerKey)
        tc.releaseOnePerKey = p.releaseOnePerKey;
}

inline void doUpdateGroupTriggerConditions(const engine::GroupTriggerConditions &payload,
                                           engine::Engine &engine, MessageController &cont)
{
    auto lead = engine.getSelectionManager()->currentLeadGroup(engine);
    if (!lead.has_value())
        return;

    std::vector<selection::SelectionManager::ZoneAddress> targets{*lead};
    for (const auto &ga : engine.getSelectionManager()->currentlySelectedGroups())
        if (ga != *lead)
            targets.push_back(ga);

    const auto &leadGrp = engine.getPatch()->getPart(lead->part)->getGroup(lead->group);
    auto delta = diffGroupTriggers(leadGrp->triggerConditions, payload);

    undo::pushPayloadUndoFor<undo::GroupTriggerConditionsSpec>(engine, targets);

    cont.scheduleAudioThreadCallback(
        [p = payload, tg = targets, delta](auto &eng) {
            for (const auto &ga : tg)
            {
                auto &grp = eng.getPatch()->getPart(ga.part)->getGroup(ga.group);
                if (ga == tg.front())
                    grp->triggerConditions = p;
                else
                    applyGroupTriggerDelta(grp->triggerConditions, p, delta);
                grp->triggerConditions.setupOnUnstream(
                    eng.getPatch()->getPart(ga.part)->groupTriggerInstrumentState);
            }
            // a group selection lives in one part, so its latches settle in one pass
            eng.getPatch()->getPart(tg.front().part)->guaranteeKeyswitchLatchCoherence(eng);
        },
        [part = lead->part](const auto &eng) {
            // the edit may have moved a switch key or the live articulation
            eng.sendKeySwitchStateToClient((int16_t)part);
        });
}

CLIENT_TO_SERIAL(UpdateGroupTriggerConditions, c2s_update_group_trigger_conditions,
                 scxt::engine::GroupTriggerConditions,
                 doUpdateGroupTriggerConditions(payload, engine, cont));

inline void doCopyGroupTriggersLeadToAll(engine::Engine &engine)
{
    auto &sm = engine.getSelectionManager();
    if (!sm->currentLeadGroup(engine).has_value())
        return;

    std::vector<selection::SelectionManager::ZoneAddress> targets;
    for (const auto &ga : sm->currentlySelectedGroups())
        targets.push_back(ga);
    undo::pushPayloadUndoFor<undo::GroupTriggerConditionsSpec>(engine, targets);

    sm->copyGroupTriggerStructureLeadToAll();
}
CLIENT_TO_SERIAL(CopyGroupTriggersLeadToAll, c2s_copy_group_triggers_lead_to_all, bool,
                 doCopyGroupTriggersLeadToAll(engine));

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
