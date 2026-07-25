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
#include "voice/voice.h"
#include "json/engine_traits.h"

/*
 * Group keyswitch triggers. Like the exclusive group tests these drive the engine
 * directly on the test thread so we exercise real findZone / voice creation logic.
 *
 * Layout used throughout: two groups whose zones cover the SAME range (48-72), so the
 * only thing deciding who sounds is the keyswitch. Switch keys sit below that range so
 * a switch press can never be confused with a zone hit.
 */

static constexpr double TEST_SAMPLE_RATE = 48000.0;

static constexpr int SW_A{24}, SW_B{25};
static constexpr int PLAY_KEY{60};

static scxt::engine::Engine *makeEngine()
{
    auto *e = new scxt::engine::Engine();
    e->prepareToPlay(TEST_SAMPLE_RATE);
    // Pin tuning so an MTS-ESP master on the dev box can't remap our test keys
    e->midikeyRetuner.setTuningMode(scxt::tuning::MidikeyRetuner::TWELVE_TET);
    return e;
}

static void addBlankZoneToGroup(scxt::engine::Part &part, int groupIdx, int keyLo, int keyHi)
{
    auto z = std::make_unique<scxt::engine::Zone>();
    z->mapping.keyboardRange.keyStart = keyLo;
    z->mapping.keyboardRange.keyEnd = keyHi;
    z->mapping.velocityRange.velStart = 0;
    z->mapping.velocityRange.velEnd = 127;
    z->initialize();
    part.getGroup(groupIdx)->addZone(z);
}

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

static int countLiveVoicesInGroup(const scxt::engine::Group &grp)
{
    int n = 0;
    for (const auto &zone : grp.getZones())
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            const auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned && v->terminationSequence < 0)
                n++;
        }
    return n;
}

// Voices for one specific key. A released voice still rings, so tests that press several
// keys in sequence have to ask about the key they care about rather than the whole group.
static int countLiveVoicesForKey(const scxt::engine::Group &grp, int key)
{
    int n = 0;
    for (const auto &zone : grp.getZones())
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            const auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned && v->terminationSequence < 0 && v->key == key)
                n++;
        }
    return n;
}

TEST_CASE("Keyswitch latch - exactly one articulation is live at rest", "[keyswitch]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoLatchGroups(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    // Coherence leaves the first keyswitch group selected
    REQUIRE(!part.getGroup(0)->outputInfo.mutedByLatch);
    REQUIRE(part.getGroup(1)->outputInfo.mutedByLatch);

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
    REQUIRE(part.getGroup(0)->outputInfo.mutedByLatch);
    REQUIRE(!part.getGroup(1)->outputInfo.mutedByLatch);
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
    REQUIRE(!part.getGroup(1)->outputInfo.mutedByLatch);

    eng->processNoteOnEvent(0, 0, SW_A, -1, 1.f, 0.f);
    REQUIRE(!part.getGroup(0)->outputInfo.mutedByLatch);
    REQUIRE(part.getGroup(1)->outputInfo.mutedByLatch);

    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(0)) >= 1);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(1)) == 0);
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
    REQUIRE(!part.getGroup(0)->outputInfo.mutedByLatch);

    // Pressing SW_B while the tuning remaps it into the middle of the zone range must still
    // read as a keyswitch: no voices, and group 1 becomes the live articulation. Before the
    // conditions took a MIDI key this saw 60, missed the switch, and played a note instead.
    auto switched = eng->findZone(0, 60, SW_B, -1, 100, buf);
    REQUIRE(switched == 0);
    REQUIRE(part.getGroup(0)->outputInfo.mutedByLatch);
    REQUIRE(!part.getGroup(1)->outputInfo.mutedByLatch);
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
    REQUIRE(part.getGroup(0)->outputInfo.mutedByLatch);
    REQUIRE(!part.getGroup(1)->outputInfo.mutedByLatch);

    auto s = tao::json::to_string(scxt::json::scxt_value(part.getGroup(1)->outputInfo));
    REQUIRE(s.find("mutedByLatch") != std::string::npos);

    scxt::engine::Group::GroupOutputInfo readBack;
    {
        tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
            consumer;
        tao::json::events::from_string(consumer, s);
        consumer.value.to(readBack);
    }
    REQUIRE(!readBack.mutedByLatch);

    auto s0 = tao::json::to_string(scxt::json::scxt_value(part.getGroup(0)->outputInfo));
    scxt::engine::Group::GroupOutputInfo readBack0;
    {
        tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
            consumer;
        tao::json::events::from_string(consumer, s0);
        consumer.value.to(readBack0);
    }
    REQUIRE(readBack0.mutedByLatch);
}
