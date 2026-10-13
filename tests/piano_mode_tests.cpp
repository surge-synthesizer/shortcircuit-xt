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

#include <filesystem>

#include "engine/engine.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "test_utils.h"

/*
 * Piano mode is poly, except a restruck key ends what that group has
 * sounding on the key and starts fresh voices, so zone selection runs again. GH #2743.
 */

// Named, not anonymous: a unity build would fold these helpers in with the identically
// shaped ones in the other engine test files
namespace pianomode_test
{
using pm_t = scxt::engine::Group::PlayMode;

static constexpr int PLAY_KEY{60};

// long enough that a released voice is still ringing a few blocks later
static constexpr float SLOW_RELEASE{0.5f};

// a blank zone's voice ends on its own within a block, so these play a real sample
static void addSampleZone(scxt::engine::Engine &eng, scxt::engine::Group &g, int velLo = 0,
                          int velHi = 127)
{
    auto z = std::make_unique<scxt::engine::Zone>();
    z->mapping.keyboardRange = {48, 72};
    z->mapping.velocityRange = {velLo, velHi};
    z->mapping.rootKey = 60;
    z->initialize();
    z->egStorage[0].r = SLOW_RELEASE;
    g.addZone(z);
    auto *zp = g.getZone(g.getZones().size() - 1).get();

    auto p = samplePath("WavStereo48k.wav");
    REQUIRE(std::filesystem::exists(p));

    // loadSampleByPath asserts it is on the serial thread; we are the only thread
    auto bypass = eng.getMessageController()->threadingChecker.bypassChecksInScope();
    auto sid = eng.getSampleManager()->loadSampleByPath(p);
    REQUIRE(sid.has_value());

    zp->variantData.variants[0].sampleID = *sid;
    zp->variantData.variants[0].active = true;
    // ENDPOINTS only, MAPPING would let the wav's chunks overwrite our key range
    REQUIRE(zp->attachToSample(*eng.getSampleManager(), 0,
                               scxt::engine::Zone::SampleInformationRead::ENDPOINTS));
}

static scxt::engine::Group &addGroup(scxt::engine::Engine &eng, pm_t pm, bool velocitySplit = false)
{
    auto &part = *eng.getPatch()->getPart(0);
    part.addGroup();
    auto &g = *part.getGroup(part.getGroups().size() - 1);
    if (velocitySplit)
    {
        addSampleZone(eng, g, 0, 63);
        addSampleZone(eng, g, 64, 127);
    }
    else
    {
        addSampleZone(eng, g);
    }
    g.outputInfo.playMode = pm;
    g.resetPolyAndPlaymode(eng);
    return g;
}

static int terminatingVoices(const scxt::engine::Group &grp)
{
    int n{0};
    for (const auto &zone : grp.getZones())
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            const auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned && v->terminationSequence > 0)
                n++;
        }
    return n;
}

static int liveVoicesInZone(const scxt::engine::Group &grp, int zoneIdx)
{
    int n{0};
    const auto &zone = grp.getZone(zoneIdx);
    for (int i = 0; i < (int)scxt::maxVoices; ++i)
    {
        const auto *v = zone->voiceWeakPointers[i];
        if (v && v->isVoiceAssigned && v->terminationSequence < 0)
            n++;
    }
    return n;
}

static void noteOn(scxt::engine::Engine &eng, int key, float vel = 1.f)
{
    eng.processNoteOnEvent(0, 0, key, -1, vel, 0.f);
}

static void noteOff(scxt::engine::Engine &eng, int key)
{
    eng.processNoteOffEvent(0, 0, key, -1, 0.f);
}

static void sustain(scxt::engine::Engine &eng, bool on)
{
    uint8_t data[3]{0xb0, 64, (uint8_t)(on ? 127 : 0)};
    eng.processMIDI1Event(0, data);
}

static void runBlocks(scxt::engine::Engine &eng, int n)
{
    for (int i = 0; i < n; ++i)
        eng.processAudio();
}
} // namespace pianomode_test

using namespace pianomode_test;

TEST_CASE("Piano mode stream string round trips", "[pianomode]")
{
    using grp_t = scxt::engine::Group;
    REQUIRE(grp_t::toStringPlayMode(pm_t::PIANO) == "pn");
    REQUIRE(grp_t::fromStringPlayMode("pn") == pm_t::PIANO);
    REQUIRE(grp_t::fromStringPlayMode("p") == pm_t::POLY);
    REQUIRE(grp_t::isPolyphonic(pm_t::PIANO));
    REQUIRE(grp_t::isPolyphonic(pm_t::POLY));
    REQUIRE_FALSE(grp_t::isPolyphonic(pm_t::MONO));
    REQUIRE_FALSE(grp_t::isPolyphonic(pm_t::LEGATO));
}

TEST_CASE("Piano mode sets the voice manager group up", "[pianomode]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &g = addGroup(*eng, pm_t::PIANO);
    using vm_t = scxt::engine::Engine::voiceManager_t;

    auto pg = (uint64_t)&g;
    REQUIRE(eng->voiceManager.getPlaymode(pg) == vm_t::PlayMode::POLY_VOICES);
    REQUIRE(eng->voiceManager.getRepeatedKeyMode(pg) == vm_t::RepeatedKeyMode::MONO_PER_KEY);

    g.outputInfo.playMode = pm_t::POLY;
    g.resetPolyAndPlaymode(*eng);
    REQUIRE(eng->voiceManager.getRepeatedKeyMode(pg) == vm_t::RepeatedKeyMode::MULTI_VOICE);
}

TEST_CASE("Piano mode restrike replaces the voice on that key", "[pianomode]")
{
    SECTION("While still held")
    {
        std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
        auto &g = addGroup(*eng, pm_t::PIANO);

        noteOn(*eng, PLAY_KEY);
        REQUIRE(countLiveVoicesInGroup(g) == 1);
        noteOn(*eng, PLAY_KEY);
        REQUIRE(countLiveVoicesInGroup(g) == 1);
        REQUIRE(terminatingVoices(g) == 1);
    }

    SECTION("While releasing")
    {
        std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
        auto &g = addGroup(*eng, pm_t::PIANO);

        noteOn(*eng, PLAY_KEY);
        runBlocks(*eng, 4);
        noteOff(*eng, PLAY_KEY);
        runBlocks(*eng, 4);
        REQUIRE(countLiveVoicesInGroup(g) == 1);
        noteOn(*eng, PLAY_KEY);
        REQUIRE(countLiveVoicesInGroup(g) == 1);
        REQUIRE(terminatingVoices(g) == 1);
    }

    SECTION("Poly stacks for comparison")
    {
        std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
        auto &g = addGroup(*eng, pm_t::POLY);

        noteOn(*eng, PLAY_KEY);
        noteOn(*eng, PLAY_KEY);
        REQUIRE(countLiveVoicesInGroup(g) == 2);
        REQUIRE(terminatingVoices(g) == 0);
    }

    SECTION("Other keys are left alone")
    {
        std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
        auto &g = addGroup(*eng, pm_t::PIANO);

        noteOn(*eng, PLAY_KEY);
        noteOn(*eng, PLAY_KEY + 4);
        noteOn(*eng, PLAY_KEY);
        REQUIRE(countLiveVoicesInGroup(g) == 2);
        REQUIRE(countLiveVoicesForKey(g, PLAY_KEY + 4) == 1);
    }
}

TEST_CASE("Piano mode does not pile up voices under the sustain pedal", "[pianomode]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &g = addGroup(*eng, pm_t::PIANO);

    sustain(*eng, true);
    for (int i = 0; i < 4; ++i)
    {
        noteOn(*eng, PLAY_KEY);
        runBlocks(*eng, 2);
        noteOff(*eng, PLAY_KEY);
        runBlocks(*eng, 2);
    }
    REQUIRE(countLiveVoicesInGroup(g) == 1);

    // the same playing in poly holds every strike
    std::unique_ptr<scxt::engine::Engine> polyEng(makeEngine());
    auto &pg = addGroup(*polyEng, pm_t::POLY);
    sustain(*polyEng, true);
    for (int i = 0; i < 4; ++i)
    {
        noteOn(*polyEng, PLAY_KEY);
        runBlocks(*polyEng, 2);
        noteOff(*polyEng, PLAY_KEY);
        runBlocks(*polyEng, 2);
    }
    REQUIRE(countLiveVoicesInGroup(pg) == 4);
}

TEST_CASE("Piano mode restrike picks its zone again", "[pianomode]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &g = addGroup(*eng, pm_t::PIANO, true);

    noteOn(*eng, PLAY_KEY, 0.3f);
    REQUIRE(liveVoicesInZone(g, 0) == 1);
    REQUIRE(liveVoicesInZone(g, 1) == 0);

    noteOn(*eng, PLAY_KEY, 0.9f);
    REQUIRE(liveVoicesInZone(g, 0) == 0);
    REQUIRE(liveVoicesInZone(g, 1) == 1);
}

TEST_CASE("Piano mode layered with a poly group", "[pianomode]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &piano = addGroup(*eng, pm_t::PIANO);
    auto &poly = addGroup(*eng, pm_t::POLY);

    noteOn(*eng, PLAY_KEY);
    noteOn(*eng, PLAY_KEY);
    REQUIRE(countLiveVoicesInGroup(piano) == 1);
    REQUIRE(countLiveVoicesInGroup(poly) == 2);
}
