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
#include <vector>

#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "test_utils.h"

/*
 * Glide (portamento) in MONO mode does not move the playing voice the way LEGATO
 * does — the voice manager terminates it and creates a new one, handing the new
 * voice the outgoing voice's sounding pitch as continuation data. These tests fence
 * that handoff, which used to be applied only to sample-less zones: as soon as a
 * sample was loaded into the zone, glide silently stopped working.
 *
 * So the sampled cases here are the real regression; the sample-less ones are the
 * control that always passed.
 */

namespace fs = std::filesystem;

namespace
{
// 0..1 on the 25-second exp scale; ~32ms, so a glide resolves in ~100 blocks.
constexpr float SLOW_GLIDE = 0.3f;

/*
 * One part / one group / n zones all spanning the keys we play, driven directly on the
 * test thread (no ConsoleHarness, no audio thread) exactly like exclusive_group_tests.
 * With n > 1 the zones layer, so one note-on makes one voice per zone.
 */
struct GlideFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Group *group{nullptr};
    scxt::engine::Zone *zone{nullptr};
    std::vector<scxt::engine::Zone *> zones;

    explicit GlideFixture(bool withSample, int zoneCount = 1)
    {
        eng = std::make_unique<scxt::engine::Engine>();
        eng->prepareToPlay(TEST_SAMPLE_RATE);
        // Pin to 12-TET so an MTS-ESP master on the dev box can't retune the keys.
        eng->midikeyRetuner.setTuningMode(scxt::tuning::MidikeyRetuner::TWELVE_TET);

        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        group = part.getGroup(0).get();

        for (int i = 0; i < zoneCount; ++i)
        {
            auto z = std::make_unique<scxt::engine::Zone>();
            z->mapping.keyboardRange = {48, 84};
            z->mapping.velocityRange = {0, 127};
            z->mapping.rootKey = 60;
            z->initialize();
            group->addZone(z);
            zones.push_back(group->getZone(i).get());

            if (withSample)
                loadSample(zones.back());
            else
                addOscillator(zones.back());
        }
        zone = zones.front();
    }

    /*
     * A zone with no sample only sustains if something else is making sound — without a
     * processor the voice ends on its first block (no generator, no procs). An oscillator
     * is also what "empty zone as a VA synth" actually means in practice.
     */
    void addOscillator(scxt::engine::Zone *z)
    {
        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        z->setProcessorType(0, scxt::dsp::processor::proct_osc_sineplus);
    }

    void loadSample(scxt::engine::Zone *z)
    {
        auto p = samplePath("WavStereo48k.wav");
        REQUIRE(fs::exists(p));

        // loadSampleByPath asserts it is on the serial thread; we are the only thread.
        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        auto sid = eng->getSampleManager()->loadSampleByPath(p);
        REQUIRE(sid.has_value());

        z->variantData.variants[0].sampleID = *sid;
        z->variantData.variants[0].active = true;
        // ENDPOINTS only — MAPPING would let the wav's chunks overwrite our key range.
        REQUIRE(z->attachToSample(*eng->getSampleManager(), 0,
                                  scxt::engine::Zone::SampleInformationRead::ENDPOINTS));
        REQUIRE(z->getNumSampleLoaded() == 1);
    }

    void setMonoWithGlide(float glideTime)
    {
        group->outputInfo.playMode = scxt::engine::Group::PlayMode::MONO;
        group->outputInfo.notePriority = scxt::engine::Group::NotePriority::LATEST;
        group->outputInfo.glideTime = glideTime;
        group->resetPolyAndPlaymode(*eng);
    }

    void noteOn(int key) { eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f); }
    void noteOff(int key) { eng->processNoteOffEvent(0, 0, key, -1, 0.f); }

    void runBlocks(int n)
    {
        for (int i = 0; i < n; ++i)
            eng->processAudio();
    }

    // Every live (non-choked) voice sounding a given key, across all the layered zones.
    std::vector<scxt::voice::Voice *> voicesForKey(int key) const
    {
        std::vector<scxt::voice::Voice *> res;
        for (auto *z : zones)
        {
            for (int i = 0; i < (int)scxt::maxVoices; ++i)
            {
                auto *v = z->voiceWeakPointers[i];
                if (v && v->isVoiceAssigned && v->isVoicePlaying && v->terminationSequence < 0 &&
                    (int)v->key == key)
                    res.push_back(v);
            }
        }
        return res;
    }

    scxt::voice::Voice *voiceForKey(int key) const
    {
        auto vs = voicesForKey(key);
        return vs.empty() ? nullptr : vs.front();
    }
};

/*
 * Play 60, then 72 on top of it, and report where the new voice's pitch starts.
 * pitchFloat is only written inside Voice::process, so one block has to run first.
 */
float pitchAfterMonoRetriggerTo72(GlideFixture &f)
{
    f.noteOn(60);
    f.runBlocks(20);
    REQUIRE(f.voiceForKey(60) != nullptr);

    f.noteOn(72);
    f.runBlocks(1);

    auto *v = f.voiceForKey(72);
    REQUIRE(v != nullptr);
    return v->pitchFloat;
}
} // namespace

TEST_CASE("Mono glide starts from the prior voice - sampled zone", "[glide]")
{
    // The regression: with a sample loaded the new voice used to snap straight to 72.
    GlideFixture f{true};
    f.setMonoWithGlide(SLOW_GLIDE);

    auto startPitch = pitchAfterMonoRetriggerTo72(f);
    INFO("startPitch=" << startPitch);
    CHECK(startPitch < 62.f);
    CHECK(startPitch > 59.f);

    // ...and it resolves onto the new key.
    auto *v = f.voiceForKey(72);
    REQUIRE(v != nullptr);
    f.runBlocks(300);
    CHECK(v->pitchFloat == Approx(72.f).margin(0.01f));
    CHECK_FALSE(v->inGlide);
}

TEST_CASE("Mono glide starts from the prior voice - sample-less zone", "[glide]")
{
    // Control: this path always worked.
    GlideFixture f{false};
    f.setMonoWithGlide(SLOW_GLIDE);

    auto startPitch = pitchAfterMonoRetriggerTo72(f);
    INFO("startPitch=" << startPitch);
    CHECK(startPitch < 62.f);
    CHECK(startPitch > 59.f);
}

TEST_CASE("Mono glide of zero snaps to the new key", "[glide]")
{
    GlideFixture f{true};
    f.setMonoWithGlide(0.f);

    auto startPitch = pitchAfterMonoRetriggerTo72(f);
    INFO("startPitch=" << startPitch);
    CHECK(startPitch == Approx(72.f).margin(0.01f));

    auto *v = f.voiceForKey(72);
    REQUIRE(v != nullptr);
    CHECK_FALSE(v->inGlide);
}

TEST_CASE("Mono glide on release back to a held key - sampled zone", "[glide]")
{
    /*
     * Releasing 72 while 60 is still held goes through the voice manager's
     * doMonoRetrigger, which also creates a fresh voice and also carries
     * continuation data. Same handoff, different entry point.
     */
    GlideFixture f{true};
    f.setMonoWithGlide(SLOW_GLIDE);

    f.noteOn(60);
    f.runBlocks(20);
    f.noteOn(72);
    f.runBlocks(300); // let the upward glide finish

    auto *up = f.voiceForKey(72);
    REQUIRE(up != nullptr);
    REQUIRE(up->pitchFloat == Approx(72.f).margin(0.01f));

    f.noteOff(72);
    f.runBlocks(1);

    auto *back = f.voiceForKey(60);
    REQUIRE(back != nullptr);
    INFO("pitch=" << back->pitchFloat);
    CHECK(back->pitchFloat > 70.f); // started from 72, not snapped to 60
    CHECK(back->pitchFloat < 73.f);

    f.runBlocks(300);
    CHECK(back->pitchFloat == Approx(60.f).margin(0.01f));
}

TEST_CASE("Mono glide moves every layered voice", "[glide]")
{
    /*
     * #2565: two zones layered on one key make two voices in the mono group, but the
     * voice manager only tagged the first launch entry with continuation data, so the
     * second zone snapped to the new key while the first glided.
     */
    GlideFixture f{true, 2};
    f.setMonoWithGlide(SLOW_GLIDE);

    f.noteOn(60);
    f.runBlocks(20);
    REQUIRE(f.voicesForKey(60).size() == 2);

    f.noteOn(72);
    f.runBlocks(1);

    auto vs = f.voicesForKey(72);
    REQUIRE(vs.size() == 2);
    for (auto *v : vs)
    {
        INFO("startPitch=" << v->pitchFloat);
        CHECK(v->inGlide);
        CHECK(v->pitchFloat < 62.f);
        CHECK(v->pitchFloat > 59.f);
    }

    f.runBlocks(300);
    for (auto *v : vs)
    {
        CHECK(v->pitchFloat == Approx(72.f).margin(0.01f));
        CHECK_FALSE(v->inGlide);
    }
}

TEST_CASE("Mono glide on release moves every layered voice", "[glide]")
{
    // Same layering through doMonoRetrigger: release the top note back onto a held one.
    GlideFixture f{true, 2};
    f.setMonoWithGlide(SLOW_GLIDE);

    f.noteOn(60);
    f.runBlocks(20);
    f.noteOn(72);
    f.runBlocks(300);
    REQUIRE(f.voicesForKey(72).size() == 2);

    f.noteOff(72);
    f.runBlocks(1);

    auto vs = f.voicesForKey(60);
    REQUIRE(vs.size() == 2);
    for (auto *v : vs)
    {
        INFO("pitch=" << v->pitchFloat);
        CHECK(v->inGlide);
        CHECK(v->pitchFloat > 70.f);
        CHECK(v->pitchFloat < 73.f);
    }

    f.runBlocks(300);
    for (auto *v : vs)
        CHECK(v->pitchFloat == Approx(60.f).margin(0.01f));
}

namespace
{
/*
 * Play 60 and let it go, then play 72 while 60 is still in its release tail. That prior
 * voice is sounding but not gated, which is where the glide-from setting decides.
 */
scxt::voice::Voice *playOverReleasedVoice(GlideFixture &f, scxt::engine::Group::PlayMode pm,
                                          scxt::engine::Group::GlideFrom gf)
{
    f.setMonoWithGlide(SLOW_GLIDE);
    f.group->outputInfo.playMode = pm;
    f.group->outputInfo.glideFrom = gf;
    f.group->resetPolyAndPlaymode(*f.eng);

    f.noteOn(60);
    f.runBlocks(20);
    auto *prior = f.voiceForKey(60);
    REQUIRE(prior != nullptr);
    f.noteOff(60);
    f.runBlocks(2);
    REQUIRE(prior->isVoicePlaying);
    REQUIRE_FALSE(prior->isGated);

    f.noteOn(72);
    f.runBlocks(1);
    auto *v = f.voiceForKey(72);
    REQUIRE(v != nullptr);
    return v;
}
} // namespace

TEST_CASE("Glide from gated is the default", "[glide]")
{
    scxt::engine::Group::GroupOutputInfo oi;
    CHECK(oi.glideFrom == scxt::engine::Group::GLIDE_FROM_GATED);
}

TEST_CASE("Glide from gated starts a note over a released voice on pitch", "[glide]")
{
    using grp_t = scxt::engine::Group;
    for (auto pm : {grp_t::PlayMode::MONO, grp_t::PlayMode::LEGATO})
    {
        DYNAMIC_SECTION("Play mode " << grp_t::toStringPlayMode(pm))
        {
            GlideFixture f{true};
            auto *v = playOverReleasedVoice(f, pm, grp_t::GLIDE_FROM_GATED);
            INFO("startPitch=" << v->pitchFloat);
            CHECK(v->pitchFloat == Approx(72.f).margin(0.01f));
            CHECK_FALSE(v->inGlide);
        }
    }
}

TEST_CASE("Glide from sounding glides from a released voice", "[glide]")
{
    using grp_t = scxt::engine::Group;
    for (auto pm : {grp_t::PlayMode::MONO, grp_t::PlayMode::LEGATO})
    {
        DYNAMIC_SECTION("Play mode " << grp_t::toStringPlayMode(pm))
        {
            GlideFixture f{true};
            auto *v = playOverReleasedVoice(f, pm, grp_t::GLIDE_FROM_SOUNDING);
            INFO("startPitch=" << v->pitchFloat);
            CHECK(v->inGlide);
            CHECK(v->pitchFloat < 62.f);
            CHECK(v->pitchFloat > 59.f);

            f.runBlocks(300);
            CHECK(v->pitchFloat == Approx(72.f).margin(0.01f));
        }
    }
}

TEST_CASE("Glide from gated still glides over a held note", "[glide]")
{
    using grp_t = scxt::engine::Group;
    for (auto pm : {grp_t::PlayMode::MONO, grp_t::PlayMode::LEGATO})
    {
        DYNAMIC_SECTION("Play mode " << grp_t::toStringPlayMode(pm))
        {
            GlideFixture f{true};
            f.setMonoWithGlide(SLOW_GLIDE);
            f.group->outputInfo.playMode = pm;
            f.group->resetPolyAndPlaymode(*f.eng);
            REQUIRE(f.group->outputInfo.glideFrom == grp_t::GLIDE_FROM_GATED);

            auto startPitch = pitchAfterMonoRetriggerTo72(f);
            INFO("startPitch=" << startPitch);
            CHECK(startPitch < 62.f);
            CHECK(startPitch > 59.f);
        }
    }
}
