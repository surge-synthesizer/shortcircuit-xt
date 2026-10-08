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

#include "catch2/catch2.hpp"

#include "console_harness.h"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "messaging/client/client_messages.h"
#include "selection/selection_manager.h"
#include "undo_manager/undo.h"

namespace cmsg = scxt::messaging::client;
using OI = scxt::engine::Group::GroupOutputInfo;
using GTID = scxt::engine::GroupTriggerID;

namespace
{
#define OI_OFF(f) (ptrdiff_t)offsetof(OI, f)

/*
 * Three groups in part 0, all selected, the middle one leading. Each starts with its own output
 * values so a test can tell "took the lead's value" apart from "was already that", and can catch
 * a field being reset that nobody edited.
 */
struct ThreeGroups
{
    scxt::clients::console_ui::ConsoleHarness th;
    static constexpr int lead{1};

    ThreeGroups()
    {
        th.start();
        th.stepUI();
        for (int i = 0; i < 3; ++i)
            send(cmsg::CreateGroup(0));
        REQUIRE(part().getGroups().size() == 3);

        reselectAll();

        const auto &sm = th.engine->getSelectionManager();
        REQUIRE(sm->currentlySelectedGroups().size() == 3);
        REQUIRE(sm->currentLeadGroup(*th.engine)->group == lead);

        for (int g = 0; g < 3; ++g)
        {
            auto &o = info(g);
            o.amplitude = 1.f - 0.25f * g;
            o.pan = -0.5f + 0.5f * g;
            o.tuning = 1.f * g;
        }
    }

    // forZone has to be false or a lone group action is expanded into its zones, and these have
    // none
    static scxt::selection::SelectionManager::SelectActionContents groupAction(int g, bool distinct,
                                                                               bool asLead)
    {
        scxt::selection::SelectionManager::SelectActionContents a{0, g, -1, true, distinct, asLead};
        a.forZone = false;
        return a;
    }

    void reselectAll()
    {
        send(cmsg::ApplySelectActions({groupAction(lead, true, true)}));
        for (int g = 0; g < 3; ++g)
            if (g != lead)
                send(cmsg::ApplySelectActions({groupAction(g, false, false)}));
    }

    scxt::engine::Part &part() { return *th.engine->getPatch()->getPart(0); }
    OI &info(int g) { return part().getGroup(g)->outputInfo; }
    scxt::engine::GroupTriggerConditions &cond(int g)
    {
        return part().getGroup(g)->triggerConditions;
    }

    template <typename T> void send(const T &msg, size_t drainSteps = 30)
    {
        th.sendToSerialization(msg);
        th.stepUI(drainSteps);
    }

    void sendUndo(size_t drainSteps = 30) { send(cmsg::Undo(true), drainSteps); }

    // the mix of gain and pan an edit to some other field must leave alone
    struct Levels
    {
        float amplitude, pan, tuning;
    };
    Levels levelsOf(int g) { return {info(g).amplitude, info(g).pan, info(g).tuning}; }
    void requireLevelsUnchanged(int g, const Levels &l)
    {
        INFO("group " << g);
        REQUIRE(info(g).amplitude == l.amplitude);
        REQUIRE(info(g).pan == l.pan);
        REQUIRE(info(g).tuning == l.tuning);
    }

    // the whole selection's conditions, as the client would post them back
    scxt::engine::GroupTriggerConditions leadCondCopy() { return cond(lead); }
};
} // namespace

TEST_CASE("Group settings menus span the selection", "[group-settings]")
{
    SECTION("play mode reaches every selected group")
    {
        ThreeGroups f;
        std::array<ThreeGroups::Levels, 3> before{f.levelsOf(0), f.levelsOf(1), f.levelsOf(2)};

        f.info(f.lead).playMode = scxt::engine::Group::PlayMode::MONO;
        f.send(cmsg::UpdateGroupOutputInt32TValue(
            {OI_OFF(playMode), (int32_t)f.info(f.lead).playMode}));

        for (int g = 0; g < 3; ++g)
        {
            INFO("group " << g);
            REQUIRE(f.info(g).playMode == scxt::engine::Group::PlayMode::MONO);
            f.requireLevelsUnchanged(g, before[g]);
        }
    }

    SECTION("note priority and exclusive group reach every selected group")
    {
        ThreeGroups f;
        f.info(f.lead).notePriority = scxt::engine::Group::NotePriority::HIGHEST;
        f.send(cmsg::UpdateGroupOutputInt32TValue(
            {OI_OFF(notePriority), (int32_t)f.info(f.lead).notePriority}));
        f.info(f.lead).exclusiveGroup = 4;
        f.send(cmsg::UpdateGroupOutputInt32TValue({OI_OFF(exclusiveGroup), 4}));

        for (int g = 0; g < 3; ++g)
        {
            INFO("group " << g);
            REQUIRE(f.info(g).notePriority == scxt::engine::Group::NotePriority::HIGHEST);
            REQUIRE(f.info(g).exclusiveGroup == 4);
        }
    }

    SECTION("glide from reaches every selected group and undoes")
    {
        using grp_t = scxt::engine::Group;
        ThreeGroups f;
        f.send(cmsg::UpdateGroupOutputInt32TValue(
            {OI_OFF(glideFrom), (int32_t)grp_t::GLIDE_FROM_SOUNDING}));

        for (int g = 0; g < 3; ++g)
        {
            INFO("group " << g);
            REQUIRE(f.info(g).glideFrom == grp_t::GLIDE_FROM_SOUNDING);
        }

        f.sendUndo();
        for (int g = 0; g < 3; ++g)
        {
            INFO("group " << g);
            REQUIRE(f.info(g).glideFrom == grp_t::GLIDE_FROM_GATED);
        }
    }

    SECTION("a poly limit reaches every selected group and undoes")
    {
        ThreeGroups f;
        f.send(cmsg::UpdateGroupOutputInt32TValue({OI_OFF(polyLimit), 12}));
        f.send(cmsg::UpdateGroupOutputBoolValue({OI_OFF(hasIndependentPolyLimit), true}));

        for (int g = 0; g < 3; ++g)
        {
            INFO("group " << g);
            REQUIRE(f.info(g).hasIndependentPolyLimit);
            REQUIRE(f.info(g).polyLimit == 12);
        }

        // the menu pick is two fields so it costs two steps; both come back
        f.sendUndo();
        f.sendUndo();
        for (int g = 0; g < 3; ++g)
        {
            INFO("group " << g);
            REQUIRE(!f.info(g).hasIndependentPolyLimit);
            REQUIRE(f.info(g).polyLimit != 12);
        }
    }
}

TEST_CASE("Group trigger edits span the selection", "[group-settings]")
{
    SECTION("a condition set on the lead reaches the whole selection")
    {
        ThreeGroups f;
        auto c = f.leadCondCopy();
        c.storage[0].id = GTID::KEYSWITCH_MOMENTARY;
        c.storage[0].args[0] = 48;
        f.send(cmsg::UpdateGroupTriggerConditions(c));

        for (int g = 0; g < 3; ++g)
        {
            INFO("group " << g);
            REQUIRE(f.cond(g).storage[0].id == GTID::KEYSWITCH_MOMENTARY);
            REQUIRE(f.cond(g).storage[0].args[0] == 48);
        }
    }

    SECTION("a round robin ordinal stays with its own group")
    {
        ThreeGroups f;

        // put all three in one cycle, then give each its own ordinal
        auto c = f.leadCondCopy();
        c.storage[0].id = GTID::ROUND_ROBIN_CYCLE;
        c.storage[0].args[0] = 1;
        f.send(cmsg::UpdateGroupTriggerConditions(c));
        for (int g = 0; g < 3; ++g)
        {
            REQUIRE(f.cond(g).storage[0].id == GTID::ROUND_ROBIN_CYCLE);
            f.cond(g).storage[0].args[1] = (float)(g + 1);
            f.cond(g).setupOnUnstream(f.part().groupTriggerInstrumentState);
        }

        // moving the lead's ordinal must not collapse the cycle onto one number
        c = f.leadCondCopy();
        c.storage[0].args[1] = 3;
        f.send(cmsg::UpdateGroupTriggerConditions(c));

        REQUIRE(f.cond(f.lead).storage[0].args[1] == 3);
        REQUIRE(f.cond(0).storage[0].args[1] == 1);
        REQUIRE(f.cond(2).storage[0].args[1] == 3);

        // the set, which every group in a cycle does share, still travels
        c = f.leadCondCopy();
        c.storage[0].args[0] = 2;
        f.send(cmsg::UpdateGroupTriggerConditions(c));
        for (int g = 0; g < 3; ++g)
        {
            INFO("group " << g);
            REQUIRE(f.cond(g).storage[0].args[0] == 2);
        }
        REQUIRE(f.cond(0).storage[0].args[1] == 1);
    }

    SECTION("an edit to one row leaves the others alone")
    {
        ThreeGroups f;
        auto c = f.leadCondCopy();
        c.storage[0].id = GTID::PITCH_BEND;
        c.storage[1].id = GTID::PROGRAM_CHANGE;
        c.storage[1].args[0] = 7;
        f.send(cmsg::UpdateGroupTriggerConditions(c));

        c = f.leadCondCopy();
        c.storage[1].args[0] = 9;
        f.send(cmsg::UpdateGroupTriggerConditions(c));

        for (int g = 0; g < 3; ++g)
        {
            INFO("group " << g);
            REQUIRE(f.cond(g).storage[0].id == GTID::PITCH_BEND);
            REQUIRE(f.cond(g).storage[1].args[0] == 9);
        }
    }

    SECTION("an arg edit skips a group whose row is a different trigger")
    {
        ThreeGroups f;
        auto c = f.leadCondCopy();
        c.storage[0].id = GTID::PROGRAM_CHANGE;
        c.storage[0].args[0] = 7;
        f.send(cmsg::UpdateGroupTriggerConditions(c));

        // a mixed selection: group 0's first row is pitch bend
        f.cond(0).storage[0].id = GTID::PITCH_BEND;
        f.cond(0).setupOnUnstream(f.part().groupTriggerInstrumentState);
        auto pbArgs = f.cond(0).storage[0].args;

        c = f.leadCondCopy();
        c.storage[0].args[0] = 9;
        f.send(cmsg::UpdateGroupTriggerConditions(c));

        REQUIRE(f.cond(f.lead).storage[0].args[0] == 9);
        REQUIRE(f.cond(2).storage[0].args[0] == 9);
        REQUIRE(f.cond(0).storage[0].id == GTID::PITCH_BEND);
        REQUIRE(f.cond(0).storage[0].args == pbArgs);
    }
}

TEST_CASE("A mixed trigger selection is reported and can be flattened", "[group-settings]")
{
    SECTION("differing condition ids read as inconsistent")
    {
        ThreeGroups f;
        REQUIRE(f.cond(0).sameStructureAs(f.cond(1)));

        f.cond(0).storage[0].id = GTID::PITCH_BEND;
        REQUIRE(!f.cond(0).sameStructureAs(f.cond(1)));

        // reselecting drives the display send, which is what stamps the flag on the lead
        f.reselectAll();
        REQUIRE(!f.cond(f.lead).structureConsistent);

        f.cond(0).storage[0].id = f.cond(f.lead).storage[0].id;
        f.reselectAll();
        REQUIRE(f.cond(f.lead).structureConsistent);
    }

    SECTION("args alone are values, not structure")
    {
        ThreeGroups f;
        f.cond(0).storage[0].id = GTID::PROGRAM_CHANGE;
        f.cond(1).storage[0].id = GTID::PROGRAM_CHANGE;
        f.cond(2).storage[0].id = GTID::PROGRAM_CHANGE;
        f.cond(0).storage[0].args[0] = 3;
        f.cond(1).storage[0].args[0] = 9;

        REQUIRE(f.cond(0).sameStructureAs(f.cond(1)));
    }

    SECTION("active and conjunction count as structure")
    {
        ThreeGroups f;
        f.cond(0).active[1] = !f.cond(1).active[1];
        REQUIRE(!f.cond(0).sameStructureAs(f.cond(1)));

        ThreeGroups g;
        g.cond(0).conjunctions[0] = scxt::engine::GroupTriggerConditions::Conjunction::OR;
        g.cond(1).conjunctions[0] = scxt::engine::GroupTriggerConditions::Conjunction::AND;
        REQUIRE(!g.cond(0).sameStructureAs(g.cond(1)));
    }

    SECTION("make consistent takes the lead's shape but not its ordinal")
    {
        ThreeGroups f;

        auto c = f.leadCondCopy();
        c.storage[0].id = GTID::ROUND_ROBIN_CYCLE;
        c.storage[0].args[0] = 1;
        f.send(cmsg::UpdateGroupTriggerConditions(c));
        for (int g = 0; g < 3; ++g)
        {
            f.cond(g).storage[0].args[1] = (float)(g + 1);
            f.cond(g).setupOnUnstream(f.part().groupTriggerInstrumentState);
        }

        // knock one group out of shape behind the engine's back, then flatten
        f.cond(2).storage[1].id = GTID::PITCH_BEND;
        REQUIRE(!f.cond(f.lead).sameStructureAs(f.cond(2)));

        f.send(cmsg::CopyGroupTriggersLeadToAll(true));

        for (int g = 0; g < 3; ++g)
        {
            INFO("group " << g);
            REQUIRE(f.cond(f.lead).sameStructureAs(f.cond(g)));
            REQUIRE(f.cond(g).storage[0].args[1] == (float)(g + 1));
        }
    }
}
