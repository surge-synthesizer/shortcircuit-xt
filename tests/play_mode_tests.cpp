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

#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "test_utils.h"

/*
 * PlayMode::ON_RELEASE sounds the voice from note on but parks the sample at its start point
 * until the AEG releases, so the sample plays under the release rather than under the key.
 * A sample gated AEG takes its gate from the sample, so on their own the two would wait on
 * each other forever; together they mean the AEG opens in release at full level instead.
 */

namespace fs = std::filesystem;

namespace
{
namespace mm = scxt::modulation::modulators;
using zone_t = scxt::engine::Zone;
using gm_t = mm::AdsrStorage::GateMode;
using env_t = scxt::voice::Voice::ahdsrenv_t;

struct PlayModeFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Group *group{nullptr};
    zone_t *zone{nullptr};

    PlayModeFixture()
    {
        eng = std::make_unique<scxt::engine::Engine>();
        eng->prepareToPlay(TEST_SAMPLE_RATE);
        eng->midikeyRetuner.setTuningMode(scxt::tuning::MidikeyRetuner::TWELVE_TET);

        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        group = part.getGroup(0).get();

        auto z = std::make_unique<zone_t>();
        z->mapping.keyboardRange = {48, 84};
        z->mapping.velocityRange = {0, 127};
        z->mapping.rootKey = 60;
        z->initialize();
        group->addZone(z);
        zone = group->getZone(0).get();

        auto p = samplePath("WavStereo48k.wav");
        REQUIRE(fs::exists(p));

        // loadSampleByPath asserts it is on the serial thread; we are the only thread.
        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        auto sid = eng->getSampleManager()->loadSampleByPath(p);
        REQUIRE(sid.has_value());

        zone->variantData.variants[0].sampleID = *sid;
        zone->variantData.variants[0].active = true;
        // ENDPOINTS only — MAPPING would let the wav's chunks overwrite our key range.
        REQUIRE(zone->attachToSample(*eng->getSampleManager(), 0,
                                     zone_t::SampleInformationRead::ENDPOINTS));
    }

    void setPlayMode(zone_t::PlayMode pm) { zone->variantData.variants[0].playMode = pm; }
    void setAegGateMode(gm_t gm) { zone->egStorage[0].gateMode = gm; }

    int64_t startSample() const { return zone->variantData.variants[0].startSample; }

    scxt::voice::Voice *playAndFindVoice(int key = 60)
    {
        eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f);
        eng->processAudio();

        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned && v->isVoicePlaying)
                return v;
        }
        return nullptr;
    }

    void releaseAndRun(int key = 60, int blocks = 1)
    {
        eng->processNoteOffEvent(0, 0, key, -1, 0.f);
        for (int i = 0; i < blocks; ++i)
            eng->processAudio();
    }

    void run(int blocks)
    {
        for (int i = 0; i < blocks; ++i)
            eng->processAudio();
    }
};

} // namespace

TEST_CASE("On Release holds the sample until the AEG releases", "[dsp]")
{
    SECTION("A normal zone moves the read head from note on")
    {
        PlayModeFixture f;

        auto *v = f.playAndFindVoice();
        REQUIRE(v != nullptr);
        REQUIRE_FALSE(v->holdSampleUntilAegRelease);
        REQUIRE(v->GD[0].samplePos > f.startSample());
    }

    SECTION("On Release parks the read head and still sounds the voice")
    {
        PlayModeFixture f;
        f.setPlayMode(zone_t::PlayMode::ON_RELEASE);

        auto *v = f.playAndFindVoice();
        REQUIRE(v != nullptr);
        REQUIRE(v->holdSampleUntilAegRelease);

        f.run(20);
        REQUIRE(v->GD[0].samplePos == f.startSample());
        // the voice is alive and holding, not finished
        REQUIRE(v->isVoicePlaying);
        REQUIRE(v->isAEGRunning);

        // and a parked read head is silence, whatever the AEG is doing
        for (int i = 0; i < (int)scxt::blockSize; ++i)
        {
            REQUIRE(v->output[0][i] == 0.f);
            REQUIRE(v->output[1][i] == 0.f);
        }
    }

    SECTION("The read head starts moving once the AEG releases")
    {
        PlayModeFixture f;
        f.setPlayMode(zone_t::PlayMode::ON_RELEASE);

        auto *v = f.playAndFindVoice();
        REQUIRE(v != nullptr);
        f.run(20);
        REQUIRE(v->GD[0].samplePos == f.startSample());

        f.releaseAndRun();
        REQUIRE(v->GD[0].samplePos > f.startSample());
    }

    SECTION("A held note never gets there, and never sounds")
    {
        PlayModeFixture f;
        f.setPlayMode(zone_t::PlayMode::ON_RELEASE);

        auto *v = f.playAndFindVoice();
        REQUIRE(v != nullptr);
        f.run(200);
        REQUIRE(v->GD[0].samplePos == f.startSample());
        REQUIRE(v->isVoicePlaying);
    }
}

TEST_CASE("On Release with a sample gated AEG opens in release", "[dsp]")
{
    SECTION("Only the pair does this")
    {
        REQUIRE(zone_t::aegStartsInRelease(zone_t::ON_RELEASE, gm_t::SAMPLE_GATED));
        REQUIRE_FALSE(zone_t::aegStartsInRelease(zone_t::NORMAL, gm_t::SAMPLE_GATED));
        REQUIRE_FALSE(zone_t::aegStartsInRelease(zone_t::ON_RELEASE, gm_t::GATED));
        REQUIRE_FALSE(zone_t::aegStartsInRelease(zone_t::ON_RELEASE, gm_t::ONESHOT));
    }

    SECTION("The AEG opens in release at full level, so the sample plays from note on")
    {
        PlayModeFixture f;
        f.setPlayMode(zone_t::PlayMode::ON_RELEASE);
        f.setAegGateMode(gm_t::SAMPLE_GATED);

        auto *v = f.playAndFindVoice();
        REQUIRE(v != nullptr);
        // the hold is still on - it is just satisfied from the very first block
        REQUIRE(v->holdSampleUntilAegRelease);
        REQUIRE(v->aeg.stage == env_t::s_release);
        REQUIRE(v->aeg.outBlock0 > 0.9f);
        REQUIRE(v->GD[0].samplePos > f.startSample());
    }

    SECTION("And decays from there rather than holding")
    {
        PlayModeFixture f;
        f.setPlayMode(zone_t::PlayMode::ON_RELEASE);
        f.setAegGateMode(gm_t::SAMPLE_GATED);

        auto *v = f.playAndFindVoice();
        REQUIRE(v != nullptr);
        auto opened = v->aeg.outBlock0;

        // a sample gated AEG would be held open by the sample it is now playing; release isn't
        f.run(200);
        REQUIRE(v->aeg.stage == env_t::s_release);
        REQUIRE(v->aeg.outBlock0 < opened);
    }
}
