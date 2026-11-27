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

// GH #2834: zone solo with voices playing in a zone the solo leaves out

#include <filesystem>
#include "catch2/catch2.hpp"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "voice/voice.h"
#include "test_utils.h"
#include "sst/cpputils/rtsan_support.h"

namespace zone_solo_test
{
#if BUILD_IS_RTSAN
// tests run the engine on their own thread, so tell rtsan where the audio side starts and ends
struct Realtime
{
    Realtime() { __rtsan_realtime_enter(); }
    ~Realtime() { __rtsan_realtime_exit(); }
};
#else
struct Realtime
{
};
#endif

void noteOn(scxt::engine::Engine &eng, int key)
{
    Realtime rt;
    eng.processNoteOnEvent(0, 0, key, -1, 0.8, 0.f);
}

void noteOff(scxt::engine::Engine &eng, int key)
{
    Realtime rt;
    eng.processNoteOffEvent(0, 0, key, -1, 0.f);
}

void sustain(scxt::engine::Engine &eng, bool on)
{
    uint8_t data[3]{0xb0, 64, (uint8_t)(on ? 127 : 0)};
    Realtime rt;
    eng.processMIDI1Event(0, data);
}

void run(scxt::engine::Engine &eng, int n)
{
    Realtime rt;
    for (int i = 0; i < n; ++i)
        eng.processAudio();
}

// 0..1 on the exp time scale; rings for a good while after the key comes up
constexpr float SLOW_RELEASE = 0.5f;

// blank zones die on their first block, so ring out needs a real sample
scxt::engine::Zone *addSampledZone(scxt::engine::Engine &eng, scxt::engine::Group &grp, int key,
                                   float release)
{
    auto z = std::make_unique<scxt::engine::Zone>();
    z->mapping.keyboardRange.keyStart = key;
    z->mapping.keyboardRange.keyEnd = key;
    z->mapping.velocityRange.velStart = 1;
    z->mapping.velocityRange.velEnd = 127;
    z->mapping.rootKey = key;
    z->initialize();
    z->egStorage[0].r = release;
    grp.addZone(z);
    auto *zp = grp.getZone(grp.getZones().size() - 1).get();

    auto p = samplePath("WavStereo48k.wav");
    REQUIRE(std::filesystem::exists(p));
    auto bypass = eng.getMessageController()->threadingChecker.bypassChecksInScope();
    auto sid = eng.getSampleManager()->loadSampleByPath(p);
    REQUIRE(sid.has_value());
    zp->variantData.variants[0].sampleID = *sid;
    zp->variantData.variants[0].active = true;
    // endpoints only, so the wav can't move the key range
    REQUIRE(zp->attachToSample(*eng.getSampleManager(), 0,
                               scxt::engine::Zone::SampleInformationRead::ENDPOINTS));
    return zp;
}

struct Fixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Part *part{nullptr};
    scxt::engine::Group *grp{nullptr};
    scxt::engine::Zone *zoneA{nullptr}, *zoneB{nullptr};

    Fixture(scxt::engine::Group::PlayMode mode, float releaseA)
    {
        eng.reset(makeEngine());
        part = eng->getPatch()->getPart(0).get();
        part->addGroup();
        grp = part->getGroup(0).get();
        zoneA = addSampledZone(*eng, *grp, 60, releaseA);
        zoneB = addSampledZone(*eng, *grp, 64, 0.f);
        grp->outputInfo.playMode = mode;
        grp->resetPolyAndPlaymode(*eng);
    }

    // what refreshZoneSolo pushes down with only zone b selected
    void soloB()
    {
        part->zoneSoloActive = true;
        zoneB->soloedBySelection = true;
    }

    void noteOn(int key) { zone_solo_test::noteOn(*eng, key); }
    void noteOff(int key) { zone_solo_test::noteOff(*eng, key); }
    void sustain(bool on) { zone_solo_test::sustain(*eng, on); }
    void run(int n) { zone_solo_test::run(*eng, n); }

    // blocks until the engine is silent, or -1 if it never gets there
    int runUntilSilent(int maxBlocks)
    {
        for (int i = 0; i < maxBlocks; ++i)
        {
            if (eng->activeVoices == 0)
                return i;
            run(1);
        }
        return eng->activeVoices == 0 ? maxBlocks : -1;
    }

    scxt::voice::Voice *voiceOn(scxt::engine::Zone *z)
    {
        for (auto *v : z->voiceWeakPointers)
            if (v && v->isVoiceAssigned)
                return v;
        return nullptr;
    }

    // every voice is gone and no zone still points at one
    void requireAllClean()
    {
        REQUIRE(eng->activeVoices == 0);
        for (auto *z : {zoneA, zoneB})
        {
            REQUIRE(z->activeVoices == 0);
            for (auto *v : z->voiceWeakPointers)
                REQUIRE(v == nullptr);
        }
    }
};

// ten seconds of blocks, well past any release we use
constexpr int maxBlocks = 48000 * 10 / scxt::blockSize;
} // namespace zone_solo_test

TEST_CASE("Zone solo mono return to a held key that finds no zone", "[zone-solo]")
{
    auto soloed = GENERATE(true, false);
    INFO("by solo " << soloed);
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();
    addBlankZoneToGroup(part, 0, 60, 60);
    addBlankZoneToGroup(part, 0, 64, 64);
    auto &grp = *part.getGroup(0);
    grp.outputInfo.playMode = scxt::engine::Group::PlayMode::MONO;
    grp.resetPolyAndPlaymode(*eng);

    zone_solo_test::noteOn(*eng, 60);
    REQUIRE(countLiveVoicesForKey(grp, 60) == 1);

    if (soloed)
    {
        // what refreshZoneSolo pushes down with only the 64 zone selected
        part.zoneSoloActive = true;
        grp.getZone(1)->soloedBySelection = true;
    }
    else
    {
        // any other way the held key stops finding its zone
        grp.getZone(0)->mapping.keyboardRange.keyStart = 70;
        grp.getZone(0)->mapping.keyboardRange.keyEnd = 70;
    }

    zone_solo_test::noteOn(*eng, 64);
    REQUIRE(countLiveVoicesForKey(grp, 64) == 1);

    // 60 is still held, so mono goes back to it and finds no zone
    zone_solo_test::noteOff(*eng, 64);
    zone_solo_test::run(*eng, 50);

    zone_solo_test::noteOff(*eng, 60);
    zone_solo_test::run(*eng, 50);
    REQUIRE(eng->activeVoices == 0);
}

TEST_CASE("Zone solo poly note off reaches a zone the solo leaves out", "[zone-solo]")
{
    using namespace zone_solo_test;
    auto viaPedal = GENERATE(false, true);
    INFO("released via pedal " << viaPedal);
    Fixture f(scxt::engine::Group::PlayMode::POLY, 0.f);

    if (viaPedal)
        f.sustain(true);
    f.noteOn(60);
    f.run(4);
    auto *va = f.voiceOn(f.zoneA);
    REQUIRE(va);
    REQUIRE(va->isGated);

    f.soloB();
    f.noteOn(64);
    f.run(4);
    REQUIRE(f.voiceOn(f.zoneB));
    REQUIRE(f.eng->activeVoices == 2);

    f.noteOff(60);
    f.run(20);
    if (viaPedal)
    {
        // the pedal still holds a, solo or no solo
        REQUIRE(f.voiceOn(f.zoneA) == va);
        REQUIRE(f.eng->activeVoices == 2);
        f.sustain(false);
    }
    f.run(20);

    // a released and gone, b untouched
    REQUIRE(f.voiceOn(f.zoneA) == nullptr);
    REQUIRE(f.zoneA->activeVoices == 0);
    REQUIRE(f.eng->activeVoices == 1);
    REQUIRE(f.voiceOn(f.zoneB)->isGated);

    f.noteOff(64);
    REQUIRE(f.runUntilSilent(maxBlocks) >= 0);
    f.requireAllClean();
}

TEST_CASE("Zone solo ring out of a zone the solo leaves out", "[zone-solo]")
{
    using namespace zone_solo_test;
    auto soloAfterRelease = GENERATE(false, true);
    INFO("solo after release " << soloAfterRelease);
    Fixture f(scxt::engine::Group::PlayMode::POLY, SLOW_RELEASE);

    f.noteOn(60);
    f.run(4);
    if (!soloAfterRelease)
        f.soloB();
    f.noteOff(60);
    f.run(4);
    if (soloAfterRelease)
        f.soloB();

    // still ringing in its release
    f.run(20);
    auto *va = f.voiceOn(f.zoneA);
    REQUIRE(va);
    REQUIRE(!va->isGated);
    REQUIRE(va->isVoicePlaying);
    REQUIRE(va->terminationSequence < 0);

    // solo is live: a fresh 60 finds nothing and leaves the tail alone
    f.noteOn(60);
    f.run(4);
    REQUIRE(f.eng->activeVoices == 1);
    REQUIRE(f.voiceOn(f.zoneA) == va);
    REQUIRE(!va->isGated);
    f.noteOff(60);
    f.run(4);
    REQUIRE(f.voiceOn(f.zoneA) == va);

    // b plays over the tail without disturbing it
    f.noteOn(64);
    f.run(4);
    REQUIRE(f.voiceOn(f.zoneA) == va);
    REQUIRE(f.eng->activeVoices == 2);
    f.noteOff(64);

    REQUIRE(f.runUntilSilent(maxBlocks) >= 0);
    f.requireAllClean();
}

TEST_CASE("Zone solo termination of a ringing zone the solo leaves out", "[zone-solo]")
{
    using namespace zone_solo_test;
    auto soloAfterRelease = GENERATE(false, true);
    INFO("solo after release " << soloAfterRelease);
    Fixture f(scxt::engine::Group::PlayMode::MONO, SLOW_RELEASE);

    f.noteOn(60);
    f.run(4);
    if (!soloAfterRelease)
        f.soloB();
    f.noteOff(60);
    f.run(4);
    if (soloAfterRelease)
        f.soloB();
    f.run(20);
    auto *va = f.voiceOn(f.zoneA);
    REQUIRE(va);
    REQUIRE(va->isVoicePlaying);
    REQUIRE(va->terminationSequence < 0);

    // mono b cuts a's tail with a termination fade rather than a click
    f.noteOn(64);
    REQUIRE(va->terminationSequence > 0);
    auto fade = va->terminationSequence;
    REQUIRE(f.voiceOn(f.zoneB));

    f.run(fade + 4);
    REQUIRE(f.voiceOn(f.zoneA) == nullptr);
    REQUIRE(f.zoneA->activeVoices == 0);
    REQUIRE(f.eng->activeVoices == 1);
    REQUIRE(f.voiceOn(f.zoneB)->isGated);

    f.noteOff(64);
    REQUIRE(f.runUntilSilent(maxBlocks) >= 0);
    f.requireAllClean();
}

// the sanitizer builds catch a teardown that reaches a freed voice or zone list
TEST_CASE("Engine teardown with voices still playing", "[zone-solo]")
{
    using namespace zone_solo_test;
    auto mode = GENERATE(scxt::engine::Group::PlayMode::POLY, scxt::engine::Group::PlayMode::MONO);
    INFO("play mode " << (int)mode);
    Fixture f(mode, SLOW_RELEASE);

    f.noteOn(60);
    f.noteOn(64);
    f.run(4);
    f.noteOff(60);
    f.run(4);
    REQUIRE(f.eng->activeVoices > 0);
    f.eng.reset();
}
