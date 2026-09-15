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

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>

#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "engine/feature_enums.h"
#include "json/engine_traits.h"
#include "messaging/client/client_messages.h"
#include "selection/selection_manager.h"

#include "test_utils.h"

namespace cmsg = scxt::messaging::client;
using GZF = scxt::engine::GroupZoneFeatures;

namespace
{
struct MuteSoloFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    explicit MuteSoloFixture(int groups)
    {
        th.start();
        th.stepUI();
        for (int i = 0; i < groups; ++i)
            send(cmsg::CreateGroup(0));
        REQUIRE((int)part().getGroups().size() == groups);
    }

    scxt::engine::Part &part() { return *th.engine->getPatch()->getPart(0); }
    const scxt::engine::Group::GroupOutputInfo &info(int g)
    {
        return part().getGroup(g)->outputInfo;
    }

    template <typename T> void send(const T &msg)
    {
        th.sendToSerialization(msg);
        th.stepUI();
    }

    void mute(int g, bool v, cmsg::MuteOrSoloGesture gs)
    {
        send(cmsg::MuteOrSoloGroup({0, g, false, v, gs}));
    }
    void solo(int g, bool v, cmsg::MuteOrSoloGesture gs)
    {
        send(cmsg::MuteOrSoloGroup({0, g, true, v, gs}));
    }

    std::string muted()
    {
        std::string res;
        for (int i = 0; i < (int)part().getGroups().size(); ++i)
            res += info(i).muted ? "M" : ".";
        return res;
    }
    std::string soloed()
    {
        std::string res;
        for (int i = 0; i < (int)part().getGroups().size(); ++i)
            res += info(i).soloed ? "S" : ".";
        return res;
    }
};
} // namespace

TEST_CASE("Group solo mutes the rest of its part", "[mutesolo]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    auto &other = *eng->getPatch()->getPart(1);
    for (int i = 0; i < 3; ++i)
        part.addGroup();
    other.addGroup();

    auto silenced = [&part](int g) { return part.getGroup(g)->isSilencedByMuteOrSolo(); };

    part.reconfigureGroupSolo();
    REQUIRE(!silenced(0));
    REQUIRE(!silenced(1));
    REQUIRE(!silenced(2));

    SECTION("solo silences the unsoloed groups")
    {
        part.getGroup(1)->outputInfo.soloed = true;
        part.reconfigureGroupSolo();
        other.reconfigureGroupSolo();
        REQUIRE(silenced(0));
        REQUIRE(!silenced(1));
        REQUIRE(silenced(2));
        REQUIRE(part.getGroup(0)->mutedDueToSoloAway);
        REQUIRE(!part.getGroup(1)->mutedDueToSoloAway);
        REQUIRE(!other.getGroup(0)->isSilencedByMuteOrSolo());
    }

    SECTION("mute silences on its own")
    {
        part.getGroup(0)->outputInfo.muted = true;
        part.reconfigureGroupSolo();
        REQUIRE(silenced(0));
        REQUIRE(!silenced(1));
    }

    SECTION("solo beats the group's own mute")
    {
        part.getGroup(1)->outputInfo.muted = true;
        part.getGroup(1)->outputInfo.soloed = true;
        part.reconfigureGroupSolo();
        REQUIRE(silenced(0));
        REQUIRE(!silenced(1));
    }

    SECTION("removing the last solo releases the part")
    {
        part.getGroup(2)->outputInfo.soloed = true;
        part.reconfigureGroupSolo();
        REQUIRE(silenced(0));
        part.getGroup(2)->outputInfo.soloed = false;
        part.reconfigureGroupSolo();
        REQUIRE(!silenced(0));
        REQUIRE(!part.getGroup(0)->mutedDueToSoloAway);
    }
}

TEST_CASE("Group solo state follows the part process", "[mutesolo]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();
    part.addGroup();
    addBlankZoneToGroup(part, 0, 36, 47);
    addBlankZoneToGroup(part, 1, 48, 59);

    part.getGroup(1)->outputInfo.soloed = true;
    eng->processNoteOnEvent(0, 0, 40, -1, 1.f, 0.f);
    for (int i = 0; i < 4; ++i)
        eng->processAudio();

    REQUIRE(part.getGroup(0)->mutedDueToSoloAway);
    REQUIRE(!part.getGroup(1)->mutedDueToSoloAway);
}

TEST_CASE("Group soloed streams", "[mutesolo]")
{
    auto unstream = [](const std::string &s, scxt::engine::Group::GroupOutputInfo &to) {
        tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
            consumer;
        tao::json::events::from_string(consumer, s);
        consumer.value.to(to);
    };

    scxt::engine::Group::GroupOutputInfo info;
    auto plain = tao::json::to_string(scxt::json::scxt_value(info));
    REQUIRE(plain.find("soloed") == std::string::npos);

    info.soloed = true;
    auto soloed = tao::json::to_string(scxt::json::scxt_value(info));
    scxt::engine::Group::GroupOutputInfo back;
    unstream(soloed, back);
    REQUIRE(back.soloed);

    scxt::engine::Group::GroupOutputInfo fromPlain;
    fromPlain.soloed = true;
    unstream(plain, fromPlain);
    REQUIRE(!fromPlain.soloed);
}

TEST_CASE("Group mute and solo gestures", "[mutesolo]")
{
    SECTION("this group only touches the one")
    {
        MuteSoloFixture f(4);
        f.mute(1, true, cmsg::MS_THIS_GROUP);
        REQUIRE(f.muted() == ".M..");
        f.solo(2, true, cmsg::MS_THIS_GROUP);
        REQUIRE(f.soloed() == "..S.");
        REQUIRE(f.muted() == ".M..");
    }

    SECTION("selected groups follow a click on one of them")
    {
        MuteSoloFixture f(5);
        f.send(cmsg::ApplySelectActions(
            {{0, 1, -1, true, true, true}, {0, 3, -1, true, false, false}}));

        f.mute(3, true, cmsg::MS_SELECTED_GROUPS);
        REQUIRE(f.muted() == ".M.M.");

        // a click outside the selection leaves the selection alone
        f.mute(4, true, cmsg::MS_SELECTED_GROUPS);
        REQUIRE(f.muted() == ".M.MM");
    }

    SECTION("exclusive leaves only this group")
    {
        MuteSoloFixture f(4);
        f.solo(0, true, cmsg::MS_THIS_GROUP);
        f.solo(3, true, cmsg::MS_THIS_GROUP);
        REQUIRE(f.soloed() == "S..S");

        f.solo(1, true, cmsg::MS_EXCLUSIVE);
        REQUIRE(f.soloed() == ".S..");

        // the client toggle flips first, so an exclusive click on an already-on group sends false
        f.solo(3, true, cmsg::MS_THIS_GROUP);
        f.solo(1, false, cmsg::MS_EXCLUSIVE);
        REQUIRE(f.soloed() == ".S..");

        // exclusive on the only one clears everything
        f.solo(1, false, cmsg::MS_EXCLUSIVE);
        REQUIRE(f.soloed() == "....");

        f.mute(2, true, cmsg::MS_EXCLUSIVE);
        REQUIRE(f.muted() == "..M.");
    }

    SECTION("range sweeps to the nearest group already in the state")
    {
        MuteSoloFixture f(8);
        f.mute(1, true, cmsg::MS_THIS_GROUP);
        f.mute(5, true, cmsg::MS_RANGE);
        REQUIRE(f.muted() == ".MMMMM..");

        // nearest wins, so group 7 only reaches back to group 5
        f.mute(7, true, cmsg::MS_RANGE);
        REQUIRE(f.muted() == ".MMMMMMM");

        // unmuting sweeps to the nearest unmuted group
        f.mute(2, false, cmsg::MS_RANGE);
        REQUIRE(f.muted() == "...MMMMM");
    }

    SECTION("range with nothing to anchor to is just this group")
    {
        MuteSoloFixture f(4);
        f.solo(2, true, cmsg::MS_RANGE);
        REQUIRE(f.soloed() == "..S.");
    }

    SECTION("solo stamps the structure")
    {
        MuteSoloFixture f(3);
        f.solo(1, true, cmsg::MS_THIS_GROUP);
        f.mute(2, true, cmsg::MS_THIS_GROUP);

        auto pgz = f.th.engine->getPartGroupZoneStructure();
        int seen{0};
        for (const auto &row : pgz)
        {
            if (row.address.part != 0 || row.address.group < 0 || row.address.zone >= 0)
                continue;
            auto g = row.address.group;
            REQUIRE(((row.features & GZF::SOLOED) != 0) == (g == 1));
            REQUIRE(((row.features & GZF::MUTED) != 0) == (g == 2));
            seen++;
        }
        REQUIRE(seen == 3);
    }
}

TEST_CASE("Group mute and solo gestures undo as one step", "[mutesolo][undo]")
{
    MuteSoloFixture f(5);
    f.mute(0, true, cmsg::MS_THIS_GROUP);
    f.mute(3, true, cmsg::MS_RANGE);
    REQUIRE(f.muted() == "MMMM.");

    f.send(cmsg::Undo(true));
    REQUIRE(f.muted() == "M....");

    f.send(cmsg::Redo(true));
    REQUIRE(f.muted() == "MMMM.");

    f.solo(4, true, cmsg::MS_THIS_GROUP);
    f.solo(2, true, cmsg::MS_EXCLUSIVE);
    REQUIRE(f.soloed() == "..S..");
    f.send(cmsg::Undo(true));
    REQUIRE(f.soloed() == "....S");
}
