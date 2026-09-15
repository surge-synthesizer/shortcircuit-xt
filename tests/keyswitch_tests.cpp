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
#include "engine/engine.h"
#include "engine/zone.h"
#include "engine/feature_enums.h"
#include "voice/voice.h"
#include "json/engine_traits.h"
#include "json/stream.h"
#include "messaging/client/client_messages.h"
#include "console_harness.h"

#include "test_utils.h"

namespace cmsg = scxt::messaging::client;

/*
 * Group keyswitch triggers. Like the exclusive group tests these drive the engine
 * directly on the test thread so we exercise real findZone / voice creation logic.
 *
 * Layout used throughout: two groups whose zones cover the SAME range (48-72), so the
 * only thing deciding who sounds is the keyswitch. Switch keys sit below that range so
 * a switch press can never be confused with a zone hit.
 */

// makeEngine, addBlankZoneToGroup and the voice counters are in test_utils.h

static constexpr int SW_A{24}, SW_B{25};
static constexpr int PLAY_KEY{60};

static void setKeyswitch(scxt::engine::Part &part, int groupIdx, scxt::engine::GroupTriggerID id,
                         int key)
{
    auto &tc = part.getGroup(groupIdx)->triggerConditions;
    tc.storage[0].id = id;
    tc.storage[0].args[0] = (float)key;
    tc.active[0] = true;
    tc.setupOnUnstream(part.groupTriggerInstrumentState);
}

// Two articulations selected by latching keyswitches on SW_A and SW_B
static void setupTwoLatchGroups(scxt::engine::Engine &eng)
{
    auto &part = *eng.getPatch()->getPart(0);
    part.addGroup();
    part.addGroup();
    addBlankZoneToGroup(part, 0, 48, 72);
    addBlankZoneToGroup(part, 1, 48, 72);
    setKeyswitch(part, 0, scxt::engine::GroupTriggerID::KEYSWITCH_LATCH, SW_A);
    setKeyswitch(part, 1, scxt::engine::GroupTriggerID::KEYSWITCH_LATCH, SW_B);
    part.guaranteeKeyswitchLatchCoherence(eng);
}

// Read a JSON group into out. The target must already belong to a part - a detached Group
// has no engine to hand its zones on unstream.
static void unstreamGroup(const std::string &json, scxt::engine::Group &out)
{
    tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
        consumer;
    tao::json::events::from_string(consumer, json);
    consumer.value.to(out);
}

TEST_CASE("Keyswitch latch - exactly one articulation is live at rest", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    // Coherence leaves the first keyswitch group selected
    REQUIRE(!part.getGroup(0)->mutedByLatch);
    REQUIRE(part.getGroup(1)->mutedByLatch);

    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(0)) >= 1);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(1)) == 0);
}

TEST_CASE("Keyswitch latch - switch key selects its group and sounds nothing", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    // Pressing SW_B hands the articulation to group 1 and must not make a sound
    eng->processNoteOnEvent(0, 0, SW_B, -1, 1.f, 0.f);
    REQUIRE(part.getGroup(0)->mutedByLatch);
    REQUIRE(!part.getGroup(1)->mutedByLatch);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(0)) == 0);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(1)) == 0);

    // Now the same played key sounds group 1 instead of group 0
    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(0)) == 0);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(1)) >= 1);
}

TEST_CASE("Keyswitch latch - switching back and forth", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    eng->processNoteOnEvent(0, 0, SW_B, -1, 1.f, 0.f);
    REQUIRE(!part.getGroup(1)->mutedByLatch);

    eng->processNoteOnEvent(0, 0, SW_A, -1, 1.f, 0.f);
    REQUIRE(!part.getGroup(0)->mutedByLatch);
    REQUIRE(part.getGroup(1)->mutedByLatch);

    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(0)) >= 1);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(1)) == 0);
}

TEST_CASE("Keyswitch latch - two groups can share one switch key", "[keyswitch]")
{
    // A switch key selects an articulation, and an articulation may be built from more than
    // one group, so sharing a key has to bring all of them up together.
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    for (int i = 0; i < 3; ++i)
    {
        part.addGroup();
        addBlankZoneToGroup(part, i, 48, 72);
    }
    // groups 0 and 1 are both on SW_A; group 2 is the other articulation
    setKeyswitch(part, 0, scxt::engine::GroupTriggerID::KEYSWITCH_LATCH, SW_A);
    setKeyswitch(part, 1, scxt::engine::GroupTriggerID::KEYSWITCH_LATCH, SW_A);
    setKeyswitch(part, 2, scxt::engine::GroupTriggerID::KEYSWITCH_LATCH, SW_B);
    part.guaranteeKeyswitchLatchCoherence(*eng);

    // Coherence settles on SW_A, so both of its groups are live
    REQUIRE(!part.getGroup(0)->mutedByLatch);
    REQUIRE(!part.getGroup(1)->mutedByLatch);
    REQUIRE(part.getGroup(2)->mutedByLatch);

    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesForKey(*part.getGroup(0), PLAY_KEY) >= 1);
    REQUIRE(countLiveVoicesForKey(*part.getGroup(1), PLAY_KEY) >= 1);
    REQUIRE(countLiveVoicesForKey(*part.getGroup(2), PLAY_KEY) == 0);

    // Switch to SW_B and only group 2 answers
    eng->processNoteOnEvent(0, 0, SW_B, -1, 1.f, 0.f);
    REQUIRE(part.getGroup(0)->mutedByLatch);
    REQUIRE(part.getGroup(1)->mutedByLatch);
    REQUIRE(!part.getGroup(2)->mutedByLatch);

    eng->processNoteOnEvent(0, 0, PLAY_KEY + 1, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesForKey(*part.getGroup(0), PLAY_KEY + 1) == 0);
    REQUIRE(countLiveVoicesForKey(*part.getGroup(1), PLAY_KEY + 1) == 0);
    REQUIRE(countLiveVoicesForKey(*part.getGroup(2), PLAY_KEY + 1) >= 1);

    // And back again - both shared-key groups return together
    eng->processNoteOnEvent(0, 0, SW_A, -1, 1.f, 0.f);
    REQUIRE(!part.getGroup(0)->mutedByLatch);
    REQUIRE(!part.getGroup(1)->mutedByLatch);
    REQUIRE(part.getGroup(2)->mutedByLatch);
}

TEST_CASE("Keyswitch latch - coherence never leaves everything muted", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    // However we got here, an instrument with keyswitches and no articulation is unusable
    part.getGroup(0)->mutedByLatch = true;
    part.getGroup(1)->mutedByLatch = true;
    part.guaranteeKeyswitchLatchCoherence(*eng);

    REQUIRE(!part.getGroup(0)->mutedByLatch);
    REQUIRE(part.getGroup(1)->mutedByLatch);
}

TEST_CASE("Keyswitch - part reports every switch key and which is live", "[keyswitch]")
{
    // The mapping keyboard marks all of a part's switch keys, not just the selected group's,
    // and paints the live articulation differently from the rest.
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    using kss = scxt::engine::KeySwitchDisplayState;
    auto ks = part.keySwitchDisplay();
    REQUIRE(ks[SW_A] == (int32_t)kss::ACTIVE);
    REQUIRE(ks[SW_B] == (int32_t)kss::INACTIVE);
    REQUIRE(ks[PLAY_KEY] == (int32_t)kss::NOT_A_SWITCH);

    // Switching articulation swaps which of the two reads as live
    eng->processNoteOnEvent(0, 0, SW_B, -1, 1.f, 0.f);
    ks = part.keySwitchDisplay();
    REQUIRE(ks[SW_A] == (int32_t)kss::INACTIVE);
    REQUIRE(ks[SW_B] == (int32_t)kss::ACTIVE);

    // A momentary switch is a switch key, but is never reported live from a snapshot
    part.addGroup();
    addBlankZoneToGroup(part, 2, 48, 72);
    setKeyswitch(part, 2, scxt::engine::GroupTriggerID::KEYSWITCH_MOMENTARY, PLAY_KEY + 5);
    ks = part.keySwitchDisplay();
    REQUIRE(ks[PLAY_KEY + 5] == (int32_t)kss::MOMENTARY);
}

static int32_t keySwitchGroupFeatures(const scxt::engine::Engine &eng, int part, int group)
{
    for (const auto &b : eng.getPartGroupZoneStructure())
        if (b.address.part == part && b.address.group == group && b.address.zone == -1)
            return b.features;
    return 0;
}

TEST_CASE("Keyswitch - the group tree marks keyswitch groups and which are switched off",
          "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup(); // group 2 has no keyswitch
    part.addGroup();
    setKeyswitch(part, 3, scxt::engine::GroupTriggerID::KEYSWITCH_MOMENTARY, PLAY_KEY + 5);

    using gzf = scxt::engine::GroupZoneFeatures;
    auto has = [&eng](int g, int f) { return (keySwitchGroupFeatures(*eng, 0, g) & f) != 0; };

    REQUIRE(has(0, gzf::KEYSWITCHED));
    REQUIRE(!has(0, gzf::MUTED_BY_KEYSWITCH));
    REQUIRE(has(1, gzf::KEYSWITCHED));
    REQUIRE(has(1, gzf::MUTED_BY_KEYSWITCH));
    REQUIRE(!has(2, gzf::KEYSWITCHED));
    REQUIRE(!has(2, gzf::MUTED_BY_KEYSWITCH));
    // a momentary group is silent until its key is held
    REQUIRE(has(3, gzf::KEYSWITCHED));
    REQUIRE(has(3, gzf::MUTED_BY_KEYSWITCH));

    eng->processNoteOnEvent(0, 0, SW_B, -1, 1.f, 0.f);
    REQUIRE(has(0, gzf::MUTED_BY_KEYSWITCH));
    REQUIRE(!has(1, gzf::MUTED_BY_KEYSWITCH));

    // it is its own bit, not the user's mute
    REQUIRE(!has(0, gzf::MUTED));
}

static scxt::engine::GroupTriggerConditions keySwitchLatchOn(int key)
{
    scxt::engine::GroupTriggerConditions cond;
    cond.storage[0].id = scxt::engine::GroupTriggerID::KEYSWITCH_LATCH;
    cond.storage[0].args[0] = (float)key;
    return cond;
}

// the same two articulations as setupTwoLatchGroups, built the way the client builds them
static void setupTwoLatchGroupsThroughMessages(scxt::clients::console_ui::ConsoleHarness &th)
{
    // adding a zone selects it, which leads the group the condition then lands on
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 72, 0, 127}));
    th.stepUI();
    th.sendToSerialization(cmsg::UpdateGroupTriggerConditions(keySwitchLatchOn(SW_A)));
    th.stepUI();
    th.sendToSerialization(cmsg::CreateGroup(0));
    th.stepUI();
    th.sendToSerialization(cmsg::AddBlankZone({0, 1, 48, 72, 0, 127}));
    th.stepUI();
    th.sendToSerialization(cmsg::UpdateGroupTriggerConditions(keySwitchLatchOn(SW_B)));
    th.stepUI();
}

TEST_CASE("Keyswitch latch - a switch press refreshes the client", "[keyswitch]")
{
    // the latch moves on the audio thread, so the keyboard and tree only hear about it if the
    // engine tells them
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();
    setupTwoLatchGroupsThroughMessages(th);

    auto &part = *th.engine->getPatch()->getPart(0);
    REQUIRE(part.getGroups().size() == 2);
    REQUIRE(!part.getGroup(0)->mutedByLatch);
    REQUIRE(part.getGroup(1)->mutedByLatch);

    auto pressAndRelease = [&th](int key) {
        th.sendToSerialization(cmsg::NoteFromGUI({key, 1.f, true}));
        th.stepUI();
        th.sendToSerialization(cmsg::NoteFromGUI({key, 0.f, false}));
        th.stepUI();
    };

    th.editor->structureUpdateCount = 0;
    pressAndRelease(SW_B);
    for (int i = 0; i < 100 && th.editor->structureUpdateCount == 0; ++i)
        th.stepUI(1);

    REQUIRE(part.getGroup(0)->mutedByLatch);
    REQUIRE(!part.getGroup(1)->mutedByLatch);
    REQUIRE(th.editor->structureUpdateCount == 1);

    // pressing the key that is already live changes nothing, so sends nothing
    th.editor->structureUpdateCount = 0;
    pressAndRelease(SW_B);
    th.stepUI(20);
    REQUIRE(th.editor->structureUpdateCount == 0);
}

TEST_CASE("Keyswitch - switch keys are reported for display", "[keyswitch]")
{
    // The mapping keyboard paints these, working from a client-side copy whose conditions
    // are never built, so the query has to run off storage alone.
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    scxt::engine::GroupTriggerConditions uiCopy = part.getGroup(0)->triggerConditions;
    REQUIRE(uiCopy.isKeySwitchKey(SW_A));
    REQUIRE(!uiCopy.isKeySwitchKey(SW_B));
    REQUIRE(!uiCopy.isKeySwitchKey(PLAY_KEY));

    // Momentary switches are switch keys too, and an inactive row is not
    setKeyswitch(part, 1, scxt::engine::GroupTriggerID::KEYSWITCH_MOMENTARY, PLAY_KEY + 3);
    scxt::engine::GroupTriggerConditions momCopy = part.getGroup(1)->triggerConditions;
    REQUIRE(momCopy.isKeySwitchKey(PLAY_KEY + 3));

    momCopy.active[0] = false;
    REQUIRE(!momCopy.isKeySwitchKey(PLAY_KEY + 3));
}

TEST_CASE("Keyswitch latch - condition holding is not the same as playing", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    const auto &g = *part.getGroup(0);
    const auto &tc = g.triggerConditions;

    // The condition holds on its own switch key...
    REQUIRE(tc.keySwitchLatchHolds(*eng, g, 0, SW_A));
    REQUIRE(!tc.keySwitchLatchHolds(*eng, g, 0, SW_B));
    REQUIRE(!tc.keySwitchLatchHolds(*eng, g, 0, PLAY_KEY));

    // ...and holding is precisely what stops the group sounding on that key
    REQUIRE(!tc.groupShouldPlay(*eng, g, 0, SW_A));
    REQUIRE(tc.groupShouldPlay(*eng, g, 0, PLAY_KEY));
}

TEST_CASE("Keyswitch latch - follows the MIDI key, not the retuned key", "[keyswitch]")
{
    // A retuning source (MTS-ESP) remaps the key used for zone selection. A keyswitch is a
    // performance gesture on a physical key, so it must key off the untouched MIDI note.
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    std::array<scxt::engine::Engine::pathToZone_t, scxt::maxVoices> buf;

    // Playing PLAY_KEY while the tuning remaps it well down the keyboard still sounds the
    // selected group - the remap moves the pitch, not the switch.
    auto sounded = eng->findZone(0, 50, PLAY_KEY, -1, 100, buf);
    REQUIRE(sounded == 1);
    REQUIRE(!part.getGroup(0)->mutedByLatch);

    // Pressing SW_B while the tuning remaps it into the middle of the zone range must still
    // read as a keyswitch: no voices, and group 1 becomes the live articulation. Before the
    // conditions took a MIDI key this saw 60, missed the switch, and played a note instead.
    auto switched = eng->findZone(0, 60, SW_B, -1, 100, buf);
    REQUIRE(switched == 0);
    REQUIRE(part.getGroup(0)->mutedByLatch);
    REQUIRE(!part.getGroup(1)->mutedByLatch);
}

TEST_CASE("Keyswitch momentary - group sounds only while the switch is held", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();
    addBlankZoneToGroup(part, 0, 48, 72);
    setKeyswitch(part, 0, scxt::engine::GroupTriggerID::KEYSWITCH_MOMENTARY, SW_A);

    // Switch up - the group is silent
    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesForKey(*part.getGroup(0), PLAY_KEY) == 0);
    eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);

    // Hold the switch (it is outside the zone range so it makes no sound of its own)
    eng->processNoteOnEvent(0, 0, SW_A, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesForKey(*part.getGroup(0), SW_A) == 0);

    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesForKey(*part.getGroup(0), PLAY_KEY) >= 1);
    eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);

    // Release the switch and we are silent again
    eng->processNoteOffEvent(0, 0, SW_A, -1, 0.f);
    eng->processNoteOnEvent(0, 0, PLAY_KEY + 1, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesForKey(*part.getGroup(0), PLAY_KEY + 1) == 0);
}

TEST_CASE("Keyswitch latch - selection survives a stream round trip", "[keyswitch]")
{
    // Which articulation is live is performance state a player sets by hand mid-session,
    // so it has to come back with a saved DAW session or patch.
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    eng->processNoteOnEvent(0, 0, SW_B, -1, 1.f, 0.f);
    REQUIRE(part.getGroup(0)->mutedByLatch);
    REQUIRE(!part.getGroup(1)->mutedByLatch);

    // Save and reload the way a DAW session does. These tests run on the test thread rather
    // than the serialization thread, so the stream path's thread assert needs waiving.
    auto saved = scxt::json::streamEngineState(*eng);

    std::unique_ptr<scxt::engine::Engine> reloaded(makeEngine());
    {
        auto bg = reloaded->getMessageController()->threadingChecker.bypassChecksInScope();
        scxt::json::unstreamEngineState(*reloaded, saved);
    }

    auto &rpart = *reloaded->getPatch()->getPart(0);
    REQUIRE(rpart.getGroups().size() == 2);
    REQUIRE(rpart.getGroup(0)->mutedByLatch);
    REQUIRE(!rpart.getGroup(1)->mutedByLatch);
}

TEST_CASE("Keyswitch latch - selection is not part of the client-editable output info",
          "[keyswitch]")
{
    // outputInfo is assigned wholesale from the client on several settings updates, so the
    // live articulation must not travel in it or an unrelated edit would knock it over.
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    eng->processNoteOnEvent(0, 0, SW_B, -1, 1.f, 0.f);
    REQUIRE(part.getGroup(0)->mutedByLatch);

    // A stale client copy of group 0's settings, replayed the way the settings handlers do
    auto stale = part.getGroup(0)->outputInfo;
    part.getGroup(0)->outputInfo = stale;
    REQUIRE(part.getGroup(0)->mutedByLatch);
}

TEST_CASE("Keyswitch latch - reads the pre-move nested spelling", "[keyswitch]")
{
    // Patches saved while mutedByLatch lived inside outputInfo must still come back on the
    // articulation they were saved with.
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();

    std::string legacy = R"({"zones":[],"name":"legacy","outputInfo":{"amplitude":1.0,)"
                         R"("pan":0.0,"muted":false,"mutedByLatch":true}})";

    unstreamGroup(legacy, *part.getGroup(0));
    REQUIRE(part.getGroup(0)->mutedByLatch);
}

TEST_CASE("Keyswitch default - a chosen key is armed at rest", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    part.configuration.defaultKeySwitchKey = SW_B;
    setupTwoLatchGroups(*eng);

    REQUIRE(part.defaultKeySwitchLatchKey() == SW_B);
    REQUIRE(part.getGroup(0)->mutedByLatch);
    REQUIRE(!part.getGroup(1)->mutedByLatch);

    // and it is what coherence falls back to when nothing is live
    part.getGroup(0)->mutedByLatch = true;
    part.getGroup(1)->mutedByLatch = true;
    part.guaranteeKeyswitchLatchCoherence(*eng);
    REQUIRE(part.getGroup(0)->mutedByLatch);
    REQUIRE(!part.getGroup(1)->mutedByLatch);
}

TEST_CASE("Keyswitch default - a key no group uses means the lowest group", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    part.configuration.defaultKeySwitchKey = SW_B;
    setupTwoLatchGroups(*eng);

    // take the default key out of the keyswitches
    setKeyswitch(part, 1, scxt::engine::GroupTriggerID::KEYSWITCH_LATCH, SW_B + 5);
    REQUIRE(part.defaultKeySwitchLatchKey() == SW_A);

    part.getGroup(0)->mutedByLatch = true;
    part.getGroup(1)->mutedByLatch = true;
    part.guaranteeKeyswitchLatchCoherence(*eng);
    REQUIRE(!part.getGroup(0)->mutedByLatch);
    REQUIRE(part.getGroup(1)->mutedByLatch);

    // a part with no latches has no default at all
    std::unique_ptr<scxt::engine::Engine> bare(makeEngine());
    REQUIRE(bare->getPatch()->getPart(0)->defaultKeySwitchLatchKey() == -1);
}

TEST_CASE("Keyswitch default - adding a switch keeps the live articulation", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    eng->processNoteOnEvent(0, 0, SW_B, -1, 1.f, 0.f);
    REQUIRE(!part.getGroup(1)->mutedByLatch);

    // a new group arrives unmuted, so two keys look live until coherence settles them
    part.addGroup();
    addBlankZoneToGroup(part, 2, 48, 72);
    setKeyswitch(part, 2, scxt::engine::GroupTriggerID::KEYSWITCH_LATCH, SW_B + 1);
    REQUIRE(!part.getGroup(2)->mutedByLatch);
    part.guaranteeKeyswitchLatchCoherence(*eng);

    REQUIRE(part.getGroup(0)->mutedByLatch);
    REQUIRE(!part.getGroup(1)->mutedByLatch);
    REQUIRE(part.getGroup(2)->mutedByLatch);
}

TEST_CASE("Keyswitch default - selecting the default re-arms it", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    part.configuration.defaultKeySwitchKey = SW_B;
    part.selectDefaultKeySwitchArticulation();
    REQUIRE(part.getGroup(0)->mutedByLatch);
    REQUIRE(!part.getGroup(1)->mutedByLatch);

    part.configuration.defaultKeySwitchKey = -1;
    part.selectDefaultKeySwitchArticulation();
    REQUIRE(!part.getGroup(0)->mutedByLatch);
    REQUIRE(part.getGroup(1)->mutedByLatch);
}

TEST_CASE("Keyswitch default - survives a stream round trip", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    eng->getPatch()->getPart(0)->configuration.defaultKeySwitchKey = SW_B;

    auto saved = scxt::json::streamEngineState(*eng);

    std::unique_ptr<scxt::engine::Engine> reloaded(makeEngine());
    {
        auto bg = reloaded->getMessageController()->threadingChecker.bypassChecksInScope();
        scxt::json::unstreamEngineState(*reloaded, saved);
    }
    REQUIRE(reloaded->getPatch()->getPart(0)->configuration.defaultKeySwitchKey == SW_B);
    REQUIRE(reloaded->getPatch()->getPart(1)->configuration.defaultKeySwitchKey == -1);
}

TEST_CASE("Keyswitch default - changing it from the client arms it", "[keyswitch]")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();
    setupTwoLatchGroupsThroughMessages(th);

    auto &part = *th.engine->getPatch()->getPart(0);
    REQUIRE(!part.getGroup(0)->mutedByLatch);

    auto conf = part.configuration;
    conf.defaultKeySwitchKey = SW_B;
    th.sendToSerialization(cmsg::UpdatePartFullConfig({0, conf}));
    for (int i = 0; i < 100 && !part.getGroup(0)->mutedByLatch; ++i)
        th.stepUI(1);

    REQUIRE(part.configuration.defaultKeySwitchKey == SW_B);
    REQUIRE(part.getGroup(0)->mutedByLatch);
    REQUIRE(!part.getGroup(1)->mutedByLatch);

    // any other part edit leaves a hand-picked articulation alone
    th.sendToSerialization(cmsg::NoteFromGUI({SW_A, 1.f, true}));
    th.sendToSerialization(cmsg::NoteFromGUI({SW_A, 0.f, false}));
    for (int i = 0; i < 100 && part.getGroup(0)->mutedByLatch; ++i)
        th.stepUI(1);
    REQUIRE(!part.getGroup(0)->mutedByLatch);

    conf = part.configuration;
    conf.level = 0.5f;
    th.sendToSerialization(cmsg::UpdatePartFullConfig({0, conf}));
    th.stepUI(20);
    REQUIRE(part.configuration.level == 0.5f);
    REQUIRE(!part.getGroup(0)->mutedByLatch);
    REQUIRE(part.getGroup(1)->mutedByLatch);
}
