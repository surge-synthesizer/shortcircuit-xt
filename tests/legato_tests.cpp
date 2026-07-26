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
#include <memory>
#include <string>

#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#ifndef SCXT_TEST_SOURCE_DIR
#define SCXT_TEST_SOURCE_DIR ""
#endif

/*
 * A LEGATO group layers two zones on one key. If one of them runs out of sound while the key
 * is still down - a short one-shot under a long pad, say - it used to be gone for good: the
 * voice terminated and unregistered, and every later legato move found only its surviving
 * sibling and skipped creating anything.
 *
 * Such a voice now parks instead of ending, so the *same* voice is still there to be
 * re-attacked in place, and the group reaps the parked voices once nothing in it is sounding.
 *
 * Revival is deliberately limited to the release tail - the voice manager's
 * moveAndRetriggerVoice path, taken when no key is down. A legato move made while a key is
 * still held goes through moveVoice, and a played-out zone stays silent through it rather than
 * restarting its sample under the held note. These tests fence both sides of that line, and
 * the reap. GH #1895.
 */

namespace fs = std::filesystem;

namespace
{
constexpr double TEST_SAMPLE_RATE = 48000.0;

// The sample is ~14s long. Truncating one zone's playback to a few hundred samples makes its
// generator run out - and so its voice end - a handful of blocks into a held note.
constexpr int64_t SHORT_ZONE_END_SAMPLE = 512;

// 0..1 on the exp time scale; long enough that the release tail is still sounding a few
// blocks after the key comes up.
constexpr float SLOW_RELEASE = 0.5f;

fs::path samplePath(const std::string &relative)
{
    return fs::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / relative;
}

/*
 * One part / one group, driven directly on the test thread (no ConsoleHarness, no audio
 * thread) exactly like glide_tests. The group is LEGATO with a long zone and - unless
 * shortZoneOnly is set - a short one layered on the same keys.
 */
struct LegatoFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Group *group{nullptr};
    scxt::engine::Zone *longZone{nullptr};
    scxt::engine::Zone *shortZone{nullptr};

    /*
     * longRelease is on the 0..1 exp time scale. The default is instant, which keeps the "let
     * everything fall silent" tests short; the release-tail test needs an audible tail to move
     * into and asks for a slow one.
     */
    explicit LegatoFixture(bool shortZoneOnly = false, float longRelease = 0.f)
    {
        eng = std::make_unique<scxt::engine::Engine>();
        eng->prepareToPlay(TEST_SAMPLE_RATE);
        // Pin to 12-TET so an MTS-ESP master on the dev box can't retune the keys.
        eng->midikeyRetuner.setTuningMode(scxt::tuning::MidikeyRetuner::TWELVE_TET);

        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        group = part.getGroup(0).get();

        if (!shortZoneOnly)
        {
            longZone = addZone(-1);
            longZone->egStorage[0].r = longRelease;
        }
        shortZone = addZone(SHORT_ZONE_END_SAMPLE);

        group->outputInfo.playMode = scxt::engine::Group::PlayMode::LEGATO;
        group->outputInfo.notePriority = scxt::engine::Group::NotePriority::LATEST;
        group->resetPolyAndPlaymode(*eng);
    }

    // endSample < 0 plays the whole sample
    scxt::engine::Zone *addZone(int64_t endSample)
    {
        auto z = std::make_unique<scxt::engine::Zone>();
        z->mapping.keyboardRange = {48, 84};
        z->mapping.velocityRange = {0, 127};
        z->mapping.rootKey = 60;
        z->initialize();
        group->addZone(z);
        auto *zp = group->getZone(group->getZones().size() - 1).get();

        auto p = samplePath("WavStereo48k.wav");
        REQUIRE(fs::exists(p));

        // loadSampleByPath asserts it is on the serial thread; we are the only thread.
        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        auto sid = eng->getSampleManager()->loadSampleByPath(p);
        REQUIRE(sid.has_value());

        zp->variantData.variants[0].sampleID = *sid;
        zp->variantData.variants[0].active = true;
        // ENDPOINTS only - MAPPING would let the wav's chunks overwrite our key range.
        REQUIRE(zp->attachToSample(*eng->getSampleManager(), 0,
                                   scxt::engine::Zone::SampleInformationRead::ENDPOINTS));
        REQUIRE(zp->getNumSampleLoaded() == 1);

        if (endSample >= 0)
        {
            zp->variantData.variants[0].startSample = 0;
            zp->variantData.variants[0].endSample = endSample;
        }
        return zp;
    }

    void noteOn(int key) { eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f); }
    void noteOff(int key) { eng->processNoteOffEvent(0, 0, key, -1, 0.f); }

    void runBlocks(int n)
    {
        for (int i = 0; i < n; ++i)
            eng->processAudio();
    }

    // The group is monophonic, so a zone has at most one voice on it.
    scxt::voice::Voice *voiceIn(const scxt::engine::Zone *z) const
    {
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            auto *v = z->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned)
                return v;
        }
        return nullptr;
    }
};

// Hold a note until the short zone has played itself out and parked.
scxt::voice::Voice *holdUntilShortZoneParks(LegatoFixture &f, int key)
{
    f.noteOn(key);
    f.runBlocks(80);

    auto *sv = f.voiceIn(f.shortZone);
    REQUIRE(sv != nullptr);
    REQUIRE(sv->isParked);
    return sv;
}
} // namespace

TEST_CASE("Legato into a release tail revives a zone which ran out of sound", "[legato]")
{
    // A slow release on the long zone gives us a tail to play legato into.
    LegatoFixture f{false, SLOW_RELEASE};

    auto *shortVoice = holdUntilShortZoneParks(f, 60);
    auto creationId = shortVoice->voiceCreationId;

    // Lift the key. The long zone starts releasing, which keeps the group sounding and so
    // keeps the parked voice around; both voices are now ungated.
    f.noteOff(60);
    f.runBlocks(4);

    auto *longVoice = f.voiceIn(f.longZone);
    REQUIRE(longVoice != nullptr);
    REQUIRE(longVoice->isSounding());
    REQUIRE_FALSE(longVoice->isGated);
    REQUIRE(f.voiceIn(f.shortZone) == shortVoice);

    f.noteOn(64);
    f.runBlocks(1);

    auto *revived = f.voiceIn(f.shortZone);
    REQUIRE(revived != nullptr);

    // The whole point: it is the *same* voice, not a replacement.
    CHECK(revived == shortVoice);
    CHECK(revived->voiceCreationId == creationId);

    CHECK_FALSE(revived->isParked);
    CHECK(revived->isSounding());
    CHECK(revived->isGated);
    CHECK((int)revived->key == 64);

    // ...and its sample was rewound rather than left at the end point.
    CHECK(revived->GD[0].samplePos < SHORT_ZONE_END_SAMPLE);

    // The sibling was retriggered onto the new key rather than replaced.
    CHECK(f.voiceIn(f.longZone) == longVoice);
    CHECK((int)longVoice->key == 64);
}

TEST_CASE("A held-key legato move leaves a played-out zone parked", "[legato]")
{
    /*
     * The other side of the line. With 60 still down the voice manager takes moveVoice, which
     * glides without retriggering - a zone which has already played out stays silent rather
     * than restarting its sample underneath the held note. It is still parked, though, so it
     * is available to the release-tail path later.
     */
    LegatoFixture f;

    auto *shortVoice = holdUntilShortZoneParks(f, 60);

    f.noteOn(64);
    f.runBlocks(1);

    auto *sv = f.voiceIn(f.shortZone);
    REQUIRE(sv == shortVoice);
    CHECK(sv->isParked);
    CHECK_FALSE(sv->isSounding());
    // It still tracks the move, so a later revival lands on the right key.
    CHECK((int)sv->key == 64);

    // The sibling glided across as usual.
    auto *longVoice = f.voiceIn(f.longZone);
    REQUIRE(longVoice != nullptr);
    CHECK(longVoice->isSounding());
    CHECK((int)longVoice->key == 64);
}

TEST_CASE("Legato release back to a held key does not revive a parked zone", "[legato]")
{
    /*
     * Releasing 64 while 60 is still held goes through the voice manager's doMonoRetrigger.
     * 60 is down, so this is a moveVoice too - no revival.
     */
    LegatoFixture f;

    holdUntilShortZoneParks(f, 60);

    f.noteOn(64);
    f.runBlocks(20);

    auto *sv = f.voiceIn(f.shortZone);
    REQUIRE(sv != nullptr);
    REQUIRE(sv->isParked);

    f.noteOff(64);
    f.runBlocks(1);

    REQUIRE(f.voiceIn(f.shortZone) == sv);
    CHECK(sv->isParked);
    CHECK((int)sv->key == 60);
}

TEST_CASE("A parked voice is reaped once its group falls silent", "[legato]")
{
    LegatoFixture f;

    holdUntilShortZoneParks(f, 60);
    REQUIRE(f.eng->activeVoices == 2);

    f.noteOff(60);
    f.runBlocks(200);

    // The long zone releases, and with nothing left sounding the parked voice goes too.
    CHECK(f.eng->activeVoices == 0);
    CHECK(f.voiceIn(f.shortZone) == nullptr);
    CHECK(f.voiceIn(f.longZone) == nullptr);
    CHECK_FALSE(f.group->isActive());
}

TEST_CASE("A lone zone in a legato group does not park forever", "[legato]")
{
    /*
     * With nothing else sounding there is nothing to come back to, so the park lasts a single
     * block. This is the case that would leak a silent voice if the group reap were missing.
     */
    LegatoFixture f{true};

    f.noteOn(60);
    f.runBlocks(80);

    CHECK(f.eng->activeVoices == 0);
    CHECK(f.voiceIn(f.shortZone) == nullptr);
    CHECK_FALSE(f.group->isActive());
}

TEST_CASE("Parking is legato only - a mono group still ends its voices", "[legato]")
{
    LegatoFixture f;
    f.group->outputInfo.playMode = scxt::engine::Group::PlayMode::MONO;
    f.group->resetPolyAndPlaymode(*f.eng);

    f.noteOn(60);
    f.runBlocks(80);

    auto *sv = f.voiceIn(f.shortZone);
    CHECK(sv == nullptr); // ended outright, never parked
    CHECK(f.voiceIn(f.longZone) != nullptr);
    CHECK(f.eng->activeVoices == 1);
}
