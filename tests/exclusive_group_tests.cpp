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
#include "engine/zone.h"
#include "json/engine_traits.h"

/*
 * These tests drive the engine directly on the test thread (no ConsoleHarness,
 * no background audio thread) so we avoid the threading complexity of the full
 * harness while still exercising real voice-creation logic.
 */

static constexpr double TEST_SAMPLE_RATE = 48000.0;

/*
 * Build a minimal two-group engine in part 0:
 *   group 0 — blank zone covering keys 36-47
 *   group 1 — blank zone covering keys 48-59
 *
 * "Blank" means no sample loaded; the engine still creates voices for
 * sample-less zones (the generator just produces silence).
 */
static scxt::engine::Engine *makeEngine()
{
    auto *e = new scxt::engine::Engine();
    e->prepareToPlay(TEST_SAMPLE_RATE);
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

static void setupTwoGroups(scxt::engine::Engine &eng)
{
    auto &part = *eng.getPatch()->getPart(0);
    part.addGroup(); // group 0
    part.addGroup(); // group 1
    addBlankZoneToGroup(part, 0, 36, 47);
    addBlankZoneToGroup(part, 1, 48, 59);
}

/*
 * Overlapping layout for testing choke behaviour with shared key ranges:
 *   group 0 — zone 48-72  (G1)
 *   group 1 — zone 60-96  (G2)
 *
 * Keys 60-72 are covered by BOTH groups.
 * Keys 48-59 are covered only by G1.
 * Keys 73-96 are covered only by G2.
 */
static void setupOverlappingGroups(scxt::engine::Engine &eng)
{
    auto &part = *eng.getPatch()->getPart(0);
    part.addGroup(); // group 0 (G1)
    part.addGroup(); // group 1 (G2)
    addBlankZoneToGroup(part, 0, 48, 72);
    addBlankZoneToGroup(part, 1, 60, 96);
}

static void runBlocks(scxt::engine::Engine &e, int n = 4)
{
    for (int i = 0; i < n; ++i)
        e.processAudio();
}

// Count voices that have been choked (termination sequence started).
// Safe to call immediately after processNoteOnEvent — no blocks needed.
static int countTerminatingVoicesInGroup(const scxt::engine::Group &grp)
{
    int n = 0;
    for (const auto &zone : grp.getZones())
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            const auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned && v->terminationSequence > 0)
                n++;
        }
    return n;
}

// Count voices that exist and have NOT been choked.
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

TEST_CASE("Exclusive Group - basic choke", "[exclusive_group]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoGroups(*eng);

    auto &part = *eng->getPatch()->getPart(0);
    REQUIRE(part.getGroups().size() == 2);

    part.getGroup(0)->outputInfo.exclusiveGroup = 1;
    part.getGroup(1)->outputInfo.exclusiveGroup = 1;

    // Fire note in group 0 — voice V0 is created and alive.
    eng->processNoteOnEvent(0, 0, 40, -1, 1.f, 0.f);
    REQUIRE(part.getGroup(0)->getZone(0)->activeVoices == 1);

    // Fire note in group 1 immediately (before any audio blocks).
    // The choke runs synchronously inside processNoteOnEvent, so V0
    // should enter its termination sequence right now.
    eng->processNoteOnEvent(0, 0, 52, -1, 1.f, 0.f);

    REQUIRE(countTerminatingVoicesInGroup(*part.getGroup(0)) >= 1);
}

TEST_CASE("Exclusive Group - no choke when off", "[exclusive_group]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoGroups(*eng);

    auto &part = *eng->getPatch()->getPart(0);
    REQUIRE(part.getGroups().size() == 2);

    // Default exclusiveGroup == 0 — no choke.
    REQUIRE(part.getGroup(0)->outputInfo.exclusiveGroup == 0);
    REQUIRE(part.getGroup(1)->outputInfo.exclusiveGroup == 0);

    eng->processNoteOnEvent(0, 0, 40, -1, 1.f, 0.f);
    REQUIRE(part.getGroup(0)->getZone(0)->activeVoices == 1);

    eng->processNoteOnEvent(0, 0, 52, -1, 1.f, 0.f);

    // No voices in group 0 should have been choked.
    REQUIRE(countTerminatingVoicesInGroup(*part.getGroup(0)) == 0);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(0)) >= 1);
}

TEST_CASE("Exclusive Group - different IDs do not choke each other", "[exclusive_group]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupTwoGroups(*eng);

    auto &part = *eng->getPatch()->getPart(0);
    REQUIRE(part.getGroups().size() == 2);

    part.getGroup(0)->outputInfo.exclusiveGroup = 1;
    part.getGroup(1)->outputInfo.exclusiveGroup = 2; // different — no choke

    eng->processNoteOnEvent(0, 0, 40, -1, 1.f, 0.f);
    REQUIRE(part.getGroup(0)->getZone(0)->activeVoices == 1);

    eng->processNoteOnEvent(0, 0, 52, -1, 1.f, 0.f);

    REQUIRE(countTerminatingVoicesInGroup(*part.getGroup(0)) == 0);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(0)) >= 1);
}

TEST_CASE("Exclusive Group - streaming round-trip", "[exclusive_group]")
{
    using namespace scxt;

    engine::Group::GroupOutputInfo info;
    info.exclusiveGroup = 3;

    auto s = tao::json::to_string(scxt::json::scxt_value(info));
    REQUIRE(s.find("excg") != std::string::npos);

    engine::Group::GroupOutputInfo info2;
    {
        tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
            consumer;
        tao::json::events::from_string(consumer, s);
        consumer.value.to(info2);
    }
    REQUIRE(info2.exclusiveGroup == 3);
}

/*
 * Overlapping exclusive group tests
 * ─────────────────────────────────
 * G1: zone 48-72   G2: zone 60-96   both share exclusive group 1.
 * Overlap region: 60-72.
 *
 * Expected behaviour:
 *   A) 49 → 94  : only G1 covers 49, only G2 covers 94 → G1 choked (basic case, verified here)
 *   B) 48 → 65  : G1 playing at 48; press 65 (both groups respond) → G1 retains (voice at 48
 *                 survives because G1 is also triggered at 65)
 *   C) 95 → 65  : G2 playing at 95; press 65 → G2 retains (voice at 95 survives)
 *   D) nothing → 65 : both groups respond; last-in-group-order (G2) wins → only G2 plays
 */

TEST_CASE("Exclusive Group overlapping - non-overlap choke still works", "[exclusive_group]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupOverlappingGroups(*eng);

    auto &part = *eng->getPatch()->getPart(0);
    part.getGroup(0)->outputInfo.exclusiveGroup = 1;
    part.getGroup(1)->outputInfo.exclusiveGroup = 1;

    // 49 is only in G1; 94 is only in G2 — identical to the basic choke test
    eng->processNoteOnEvent(0, 0, 49, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(0)) >= 1);

    eng->processNoteOnEvent(0, 0, 94, -1, 1.f, 0.f);
    REQUIRE(countTerminatingVoicesInGroup(*part.getGroup(0)) >= 1);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(1)) >= 1);
}

TEST_CASE("Exclusive Group overlapping - G1 retains when 65 pressed while G1 playing at 48",
          "[exclusive_group]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupOverlappingGroups(*eng);

    auto &part = *eng->getPatch()->getPart(0);
    part.getGroup(0)->outputInfo.exclusiveGroup = 1;
    part.getGroup(1)->outputInfo.exclusiveGroup = 1;

    // Plant G1 voice at 48 (only G1 covers 48)
    eng->processNoteOnEvent(0, 0, 48, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(0)) >= 1);

    // Press 65 — both G1 (48-72) and G2 (60-96) fire.
    // G1's pre-existing voice at 48 must NOT be choked because G1 is also triggered at 65.
    eng->processNoteOnEvent(0, 0, 65, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(0)) >= 1);
}

TEST_CASE("Exclusive Group overlapping - G2 retains when 65 pressed while G2 playing at 95",
          "[exclusive_group]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupOverlappingGroups(*eng);

    auto &part = *eng->getPatch()->getPart(0);
    part.getGroup(0)->outputInfo.exclusiveGroup = 1;
    part.getGroup(1)->outputInfo.exclusiveGroup = 1;

    // Plant G2 voice at 95 (only G2 covers 95)
    eng->processNoteOnEvent(0, 0, 95, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(1)) >= 1);

    // Press 65 — both G1 (48-72) and G2 (60-96) fire.
    // G2's pre-existing voice at 95 must NOT be choked because G2 is also triggered at 65.
    eng->processNoteOnEvent(0, 0, 65, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(1)) >= 1);
}

TEST_CASE("Exclusive Group overlapping - from silence pressing 65 plays G2 (last group order)",
          "[exclusive_group]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupOverlappingGroups(*eng);

    auto &part = *eng->getPatch()->getPart(0);
    part.getGroup(0)->outputInfo.exclusiveGroup = 1;
    part.getGroup(1)->outputInfo.exclusiveGroup = 1;

    // No prior voices. Both groups respond to 65; G2 (highest group index) must win.
    eng->processNoteOnEvent(0, 0, 65, -1, 1.f, 0.f);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(0)) == 0);
    REQUIRE(countLiveVoicesInGroup(*part.getGroup(1)) >= 1);
}

TEST_CASE("Exclusive Group - old ZoneMappingData JSON loads without crash", "[exclusive_group]")
{
    // Patches saved before this change had "exclusiveGroup" under ZoneMappingData.
    // The field no longer exists there; it should be silently ignored on load.
    using namespace scxt;

    std::string oldJson =
        R"({"rootKey":60,"keyR":{"keyStart":0,"keyEnd":127,"fadeStart":0,"fadeEnd":0},)"
        R"("velR":{"velStart":0,"velEnd":127,"fadeStart":0,"fadeEnd":0},)"
        R"("pbDown":-1,"pbUp":-1,"exclusiveGroup":2,"velocitySens":1.0,)"
        R"("amplitude":0.0,"pan":0.0,"pitchOffset":0.0,"tracking":1.0})";

    engine::Zone::ZoneMappingData zmd;
    {
        tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
            consumer;
        tao::json::events::from_string(consumer, oldJson);
        consumer.value.to(zmd);
    }
    REQUIRE(zmd.rootKey == 60);
    REQUIRE(zmd.velocitySens == Approx(1.0));
}
