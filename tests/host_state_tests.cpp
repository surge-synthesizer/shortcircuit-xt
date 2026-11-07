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
#include "engine/engine.h"
#include "engine/part.h"
#include "console_harness.h"
#include "messaging/host_state.h"
#include "messaging/client/structure_messages.h"
#include "messaging/client/zone_messages.h"
#include "tao/json.hpp"
#include "test_utils.h"

namespace cmsg = scxt::messaging::client;
namespace hsm = scxt::messaging;
using HostSlot = scxt::messaging::MessageController::HostStateSlot;

namespace
{
struct HostFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    HostFixture()
    {
        th.start();
        th.sync();
    }

    template <typename T> void send(const T &m)
    {
        th.sendToSerialization(m);
        th.sync();
    }

    auto &engine() { return *th.engine; }
    auto &slot() { return th.engine->getMessageController()->hostStateSlot; }
    auto &part() { return th.engine->getPatch()->getPart(0); }

    std::string save()
    {
        std::string out;
        REQUIRE(hsm::hostStateSave(engine(), out));
        return out;
    }
};

// a state with one group holding a zone over the given keys
std::string hostStateWithZone(int keyLo, int keyHi)
{
    HostFixture src;
    src.send(cmsg::AddBlankZone({0, 0, keyLo, keyHi, 0, 127}));
    return src.save();
}

int hostZoneCount(HostFixture &f)
{
    if (f.part()->getGroups().empty())
        return 0;
    return (int)f.part()->getGroup(0)->getZones().size();
}
} // namespace

TEST_CASE("Host state slot state machine", "[host-state]")
{
    HostSlot s;
    REQUIRE(s.status().first == HostSlot::IDLE);
    REQUIRE(!s.heldBlob());
    REQUIRE(!s.take());

    SECTION("a load runs pending, loading, idle")
    {
        auto g = s.offer("one");
        REQUIRE(s.status() == std::make_pair(HostSlot::PENDING, g));
        REQUIRE(*s.heldBlob() == "one");

        auto t = s.take();
        REQUIRE(t);
        REQUIRE(t->first == g);
        REQUIRE(*t->second == "one");
        REQUIRE(s.status().first == HostSlot::LOADING);
        REQUIRE(*s.heldBlob() == "one");
        REQUIRE(!s.take());

        s.finish(g, true);
        REQUIRE(s.status().first == HostSlot::IDLE);
        REQUIRE(!s.heldBlob());
    }

    SECTION("a second offer replaces a blob not yet taken")
    {
        s.offer("one");
        auto g2 = s.offer("two");
        auto t = s.take();
        REQUIRE(t->first == g2);
        REQUIRE(*t->second == "two");
        // the first nudge took the newest, so the second finds nothing
        REQUIRE(!s.take());
    }

    SECTION("an older load finishing leaves a newer one held")
    {
        auto g1 = s.offer("one");
        s.take();
        auto g2 = s.offer("two");
        s.finish(g1, true);
        REQUIRE(s.status() == std::make_pair(HostSlot::PENDING, g2));
        REQUIRE(*s.heldBlob() == "two");
    }

    SECTION("a failed load holds its blob until cleared")
    {
        auto g = s.offer("bad");
        s.take();
        s.finish(g, false);
        REQUIRE(s.status().first == HostSlot::FAILED);
        REQUIRE(*s.heldBlob() == "bad");

        s.clearFailed();
        REQUIRE(s.status().first == HostSlot::IDLE);
        REQUIRE(!s.heldBlob());
    }

    SECTION("clearing a failure leaves a pending load alone")
    {
        s.offer("one");
        s.clearFailed();
        REQUIRE(s.status().first == HostSlot::PENDING);
    }
}

TEST_CASE("Host state shape check", "[host-state]")
{
    REQUIRE(hsm::looksLikeEngineState("{}"));
    REQUIRE(hsm::looksLikeEngineState("  {\"a\":1}\n"));
    // the save writes a null terminator, which a host hands back
    REQUIRE(hsm::looksLikeEngineState(std::string("{\"a\":1}\0", 8)));

    REQUIRE(!hsm::looksLikeEngineState(""));
    REQUIRE(!hsm::looksLikeEngineState("   "));
    REQUIRE(!hsm::looksLikeEngineState("{"));
    REQUIRE(!hsm::looksLikeEngineState("{\"a\":1, \"b\":"));
    REQUIRE(!hsm::looksLikeEngineState("[1, 2]"));
    REQUIRE(!hsm::looksLikeEngineState("<xml/>"));
}

TEST_CASE("A host load returns before the unstream and lands after it", "[host-state]")
{
    auto blob = hostStateWithZone(40, 50);

    HostFixture f;
    REQUIRE(hostZoneCount(f) == 0);

    uint64_t begun{0};
    f.slot().onLoadBegin = [&begun](auto g) { begun = g; };

    // serial can't run the nudge while we hold this, so the load is still pending
    uint64_t g{0};
    {
        std::lock_guard<std::mutex> lk(f.engine().modifyStructureMutex);
        REQUIRE(hsm::hostStateLoad(f.engine(), std::string(blob)));
        g = f.slot().status().second;
        REQUIRE(begun == g);
        REQUIRE(f.slot().status().first == HostSlot::PENDING);

        // a save in the window gives the host back exactly what it gave us
        std::string out;
        REQUIRE(hsm::hostStateSave(f.engine(), out));
        REQUIRE(out == blob);
    }

    f.th.sync();
    REQUIRE(f.slot().status() == std::make_pair(HostSlot::IDLE, g));
    REQUIRE(f.th.editor->lastHostStateLoaded == g);
    REQUIRE(hostZoneCount(f) == 1);
    REQUIRE(f.part()->getGroup(0)->getZone(0)->mapping.keyboardRange.keyStart == 40);

    REQUIRE(!f.th.editor->hasErrors());

    // once loaded a save streams the engine again, and that loads too
    HostFixture g2;
    REQUIRE(hsm::hostStateLoad(g2.engine(), f.save()));
    g2.th.sync();
    REQUIRE(hostZoneCount(g2) == 1);
    REQUIRE(g2.part()->getGroup(0)->getZone(0)->mapping.keyboardRange.keyEnd == 50);
}

TEST_CASE("Two quick host loads unstream only the second", "[host-state]")
{
    auto first = hostStateWithZone(40, 50);
    auto second = hostStateWithZone(70, 80);

    HostFixture f;
    {
        std::lock_guard<std::mutex> lk(f.engine().modifyStructureMutex);
        REQUIRE(hsm::hostStateLoad(f.engine(), std::string(first)));
        REQUIRE(hsm::hostStateLoad(f.engine(), std::string(second)));
        REQUIRE(f.save() == second);
    }

    f.th.sync();
    REQUIRE(f.th.editor->hostStateLoadedCount == 1);
    REQUIRE(f.th.editor->lastHostStateLoaded == f.slot().status().second);
    REQUIRE(hostZoneCount(f) == 1);
    REQUIRE(f.part()->getGroup(0)->getZone(0)->mapping.keyboardRange.keyStart == 70);
}

TEST_CASE("A blob that is not a state changes nothing", "[host-state]")
{
    HostFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 50, 0, 127}));

    bool begun{false};
    f.slot().onLoadBegin = [&begun](auto) { begun = true; };

    REQUIRE(!hsm::hostStateLoad(f.engine(), std::string("{\"truncated\": ")));
    REQUIRE(!hsm::hostStateLoad(f.engine(), std::string()));
    REQUIRE(!begun);
    REQUIRE(f.slot().status() == std::make_pair(HostSlot::IDLE, uint64_t{0}));

    f.th.sync();
    REQUIRE(hostZoneCount(f) == 1);
}

TEST_CASE("A failed host load keeps its blob until the first edit", "[host-state]")
{
    HostFixture f;

    // the shape passes, the parse doesn't
    std::string bad = "{ this is not json }";
    REQUIRE(hsm::hostStateLoad(f.engine(), std::string(bad)));
    f.th.sync();

    auto [phase, g] = f.slot().status();
    REQUIRE(phase == HostSlot::FAILED);
    REQUIRE(f.th.editor->lastHostStateLoaded == g);
    REQUIRE(f.th.editor->hasErrors());

    // an autosave now must not replace the project with whatever the engine holds
    REQUIRE(f.save() == bad);

    f.send(cmsg::AddBlankZone({0, 0, 40, 50, 0, 127}));
    REQUIRE(f.slot().status().first == HostSlot::IDLE);
    auto edited = f.save();
    REQUIRE(edited != bad);
    REQUIRE(hsm::looksLikeEngineState(edited));
}

TEST_CASE("A host load after a failure loads", "[host-state]")
{
    auto good = hostStateWithZone(60, 64);

    HostFixture f;
    REQUIRE(hsm::hostStateLoad(f.engine(), std::string("{ nope }")));
    f.th.sync();
    REQUIRE(f.slot().status().first == HostSlot::FAILED);

    REQUIRE(hsm::hostStateLoad(f.engine(), std::string(good)));
    f.th.sync();
    REQUIRE(f.slot().status().first == HostSlot::IDLE);
    REQUIRE(hostZoneCount(f) == 1);
    REQUIRE(f.part()->getGroup(0)->getZone(0)->mapping.keyboardRange.keyStart == 60);
}

TEST_CASE("Host saves while the serial thread edits stay whole", "[host-state]")
{
    HostFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 50, 0, 127}));

    // queue edits without waiting, so serial is changing structure while we save
    for (int i = 0; i < 40; ++i)
    {
        f.th.sendToSerialization(cmsg::AddBlankZone({0, 0, 40 + i, 50 + i, 0, 127}));
        f.th.sendToSerialization(cmsg::CreateGroup(0));
        f.engine().getMessageController()->wakeSerialization();

        // every save parses, so none caught the engine half changed
        auto s = f.save();
        REQUIRE_NOTHROW(tao::json::from_string(s));
    }
    REQUIRE(f.th.sync());
    REQUIRE(f.part()->getGroups().size() == 41);
}
