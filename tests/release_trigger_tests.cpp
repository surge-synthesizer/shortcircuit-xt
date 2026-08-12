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
#include "engine/group_triggers.h"
#include "engine/held_notes.h"
#include "json/engine_traits.h"

#include "test_utils.h"

/*
 * Groups which make their voices when the key comes up rather than when it goes down. The
 * press sounds nothing but is remembered, and the release plays the group at the velocity the
 * press arrived with. Its gate-following envelopes are rewritten, since nothing is holding
 * the key: the AEG follows the sample and the modulation EGs run one shot. See issue #2186.
 */

// Named, not anonymous: a unity build would fold these helpers in with the identically
// shaped ones in the other engine test files
namespace releasetrigger_test
{

namespace mm = scxt::modulation::modulators;
using VCM = scxt::engine::VoiceCreationMode;

static constexpr int PLAY_KEY{60};

static void addZone(scxt::engine::Part &part, int groupIdx, int keyLo, int keyHi, int velLo = 0,
                    int velHi = 127)
{
    auto z = std::make_unique<scxt::engine::Zone>();
    z->mapping.keyboardRange = {keyLo, keyHi};
    z->mapping.velocityRange = {velLo, velHi};
    z->mapping.rootKey = 60;
    z->initialize();
    part.getGroup(groupIdx)->addZone(z);
}

// A part with `n` groups, each holding one zone over 48..72, all triggering on note on
static scxt::engine::Part &setupGroups(scxt::engine::Engine &eng, int n)
{
    auto &part = *eng.getPatch()->getPart(0);
    for (int i = 0; i < n; ++i)
    {
        part.addGroup();
        addZone(part, i, 48, 72);
    }
    return part;
}

static void setVoiceCreation(scxt::engine::Part &part, int groupIdx, VCM mode)
{
    part.getGroup(groupIdx)->triggerConditions.voiceCreationMode = mode;
}

static int liveVoices(scxt::engine::Part &part, int groupIdx)
{
    return countLiveVoicesInGroup(*part.getGroup(groupIdx));
}

// Voices belonging to one specific zone of a group, so a velocity split can be read off
static int liveVoicesInZone(scxt::engine::Part &part, int groupIdx, int zoneIdx)
{
    int n{0};
    const auto &zone = part.getGroup(groupIdx)->getZone(zoneIdx);
    for (int i = 0; i < (int)scxt::maxVoices; ++i)
    {
        const auto *v = zone->voiceWeakPointers[i];
        if (v && v->isVoiceAssigned && v->terminationSequence < 0)
            n++;
    }
    return n;
}

static scxt::voice::Voice *firstVoiceIn(scxt::engine::Part &part, int groupIdx)
{
    for (const auto &zone : part.getGroup(groupIdx)->getZones())
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned)
                return v;
        }
    return nullptr;
}

static void midiNoteOn(scxt::engine::Engine &eng, int channel, int key, int vel)
{
    uint8_t data[3]{(uint8_t)(0x90 | channel), (uint8_t)key, (uint8_t)vel};
    eng.processMIDI1Event(0, data);
}

static void midiNoteOff(scxt::engine::Engine &eng, int channel, int key, int vel = 0)
{
    uint8_t data[3]{(uint8_t)(0x80 | channel), (uint8_t)key, (uint8_t)vel};
    eng.processMIDI1Event(0, data);
}

static void midiCC(scxt::engine::Engine &eng, int channel, int cc, int val)
{
    uint8_t data[3]{(uint8_t)(0xb0 | channel), (uint8_t)cc, (uint8_t)val};
    eng.processMIDI1Event(0, data);
}

} // namespace releasetrigger_test

using namespace releasetrigger_test;

TEST_CASE("Held notes remembers the press", "[releasetrigger]")
{
    SECTION("A MIDI 1 note round trips")
    {
        scxt::engine::HeldNotes hn;
        REQUIRE(hn.heldCount() == 0);

        hn.noteOn(0, 60, -1, 0.75f);
        REQUIRE(hn.heldCount() == 1);
        REQUIRE(hn.releaseNote(0, 60, -1) == Approx(0.75f));
        REQUIRE(hn.heldCount() == 0);
    }

    SECTION("A note which was never pressed reports nothing")
    {
        scxt::engine::HeldNotes hn;
        REQUIRE(hn.releaseNote(0, 60, -1) < 0.f);

        // and a different key doesn't answer for this one
        hn.noteOn(0, 61, -1, 0.5f);
        REQUIRE(hn.releaseNote(0, 60, -1) < 0.f);
        REQUIRE(hn.heldCount() == 1);
    }

    SECTION("Channels are kept apart")
    {
        scxt::engine::HeldNotes hn;
        hn.noteOn(0, 60, -1, 0.25f);
        hn.noteOn(3, 60, -1, 0.9f);

        REQUIRE(hn.releaseNote(3, 60, -1) == Approx(0.9f));
        REQUIRE(hn.releaseNote(0, 60, -1) == Approx(0.25f));
    }

    SECTION("CLAP note ids release independently")
    {
        scxt::engine::HeldNotes hn;
        hn.noteOn(0, 60, 11, 0.2f);
        hn.noteOn(0, 60, 12, 0.8f);
        REQUIRE(hn.heldCount() == 2);

        REQUIRE(hn.releaseNote(0, 60, 11) == Approx(0.2f));
        REQUIRE(hn.heldCount() == 1);
        REQUIRE(hn.releaseNote(0, 60, 12) == Approx(0.8f));
        REQUIRE(hn.heldCount() == 0);
    }

    SECTION("A MIDI 1 note off lets go of every press on that key")
    {
        /*
         * No note ids means no way to tell two presses of one key apart, which is exactly how
         * the voice manager sees it too - one note off releases both. The newest press is the
         * one the release plays at.
         */
        scxt::engine::HeldNotes hn;
        hn.noteOn(0, 60, -1, 0.2f);
        hn.noteOn(0, 60, -1, 0.8f);
        REQUIRE(hn.heldCount() == 2);

        REQUIRE(hn.releaseNote(0, 60, -1) == Approx(0.8f));
        REQUIRE(hn.heldCount() == 0);
    }

    SECTION("A wildcard note off matches a note which carries an id")
    {
        scxt::engine::HeldNotes hn;
        hn.noteOn(0, 60, 7, 0.4f);
        REQUIRE(hn.releaseNote(0, 60, -1) == Approx(0.4f));
        REQUIRE(hn.heldCount() == 0);
    }

    SECTION("Clearing forgets everything")
    {
        scxt::engine::HeldNotes hn;
        for (int k = 40; k < 60; ++k)
            hn.noteOn(0, k, -1, 0.5f);
        REQUIRE(hn.heldCount() == 20);

        hn.clear();
        REQUIRE(hn.heldCount() == 0);
        REQUIRE(hn.releaseNote(0, 45, -1) < 0.f);
    }

    SECTION("Overflow drops the press rather than growing")
    {
        scxt::engine::HeldNotes hn;
        for (size_t i = 0; i < scxt::engine::HeldNotes::capacity + 20; ++i)
            hn.noteOn(0, 60, (int32_t)i, 0.5f);

        REQUIRE(hn.heldCount() == scxt::engine::HeldNotes::capacity);
        REQUIRE(hn.velocityFor(0, 60, (int32_t)scxt::engine::HeldNotes::capacity + 5) < 0.f);
    }
}

TEST_CASE("A note on group is unchanged", "[releasetrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupGroups(*eng, 1);

    REQUIRE(liveVoices(part, 0) == 0);
    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    REQUIRE(liveVoices(part, 0) == 1);

    // and letting go doesn't add a second one
    eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);
    REQUIRE(liveVoices(part, 0) == 1);
    REQUIRE(firstVoiceIn(part, 0)->createdByReleaseTrigger == false);
}

TEST_CASE("A release group sounds on the way up, not the way down", "[releasetrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupGroups(*eng, 1);
    setVoiceCreation(part, 0, VCM::ON_NOTE_OFF);

    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    REQUIRE(liveVoices(part, 0) == 0);

    eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);
    REQUIRE(liveVoices(part, 0) == 1);

    auto *v = firstVoiceIn(part, 0);
    REQUIRE(v != nullptr);
    REQUIRE(v->key == PLAY_KEY);
    REQUIRE(v->createdByReleaseTrigger == true);
    // the note-off which made it also let it go
    REQUIRE(v->isGated == false);
}

TEST_CASE("A release with no press behind it sounds nothing", "[releasetrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupGroups(*eng, 1);
    setVoiceCreation(part, 0, VCM::ON_NOTE_OFF);

    eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);
    REQUIRE(liveVoices(part, 0) == 0);

    // one press is one release, not two
    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);
    REQUIRE(liveVoices(part, 0) == 1);
    eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);
    REQUIRE(liveVoices(part, 0) == 1);
}

TEST_CASE("A note on group and a release group split one key", "[releasetrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupGroups(*eng, 2);
    setVoiceCreation(part, 1, VCM::ON_NOTE_OFF);

    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    REQUIRE(liveVoices(part, 0) == 1);
    REQUIRE(liveVoices(part, 1) == 0);

    eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);
    REQUIRE(liveVoices(part, 0) == 1); // released, still ringing, not doubled
    REQUIRE(liveVoices(part, 1) == 1);
}

TEST_CASE("A release voice plays at the press velocity", "[releasetrigger]")
{
    // one group, two zones over the same keys, split by velocity
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();
    addZone(part, 0, 48, 72, 0, 63);   // zone 0: soft
    addZone(part, 0, 48, 72, 64, 127); // zone 1: loud
    setVoiceCreation(part, 0, VCM::ON_NOTE_OFF);

    SECTION("A hard press lands in the loud zone even when let go gently")
    {
        eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 0.9f, 0.f);
        eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);

        REQUIRE(liveVoicesInZone(part, 0, 0) == 0);
        REQUIRE(liveVoicesInZone(part, 0, 1) == 1);
        REQUIRE(firstVoiceIn(part, 0)->velocity == Approx(0.9f));
    }

    SECTION("A soft press lands in the soft zone even when let go hard")
    {
        eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 0.1f, 0.f);
        eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 1.f);

        REQUIRE(liveVoicesInZone(part, 0, 0) == 1);
        REQUIRE(liveVoicesInZone(part, 0, 1) == 0);
        REQUIRE(firstVoiceIn(part, 0)->velocity == Approx(0.1f));
    }
}

TEST_CASE("Release triggers arrive by MIDI 1 as well as by note event", "[releasetrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();
    addZone(part, 0, 48, 72, 0, 63);
    addZone(part, 0, 48, 72, 64, 127);
    setVoiceCreation(part, 0, VCM::ON_NOTE_OFF);

    SECTION("A note off message")
    {
        midiNoteOn(*eng, 0, PLAY_KEY, 100);
        REQUIRE(liveVoices(part, 0) == 0);

        midiNoteOff(*eng, 0, PLAY_KEY);
        REQUIRE(liveVoicesInZone(part, 0, 1) == 1); // 100 is in the loud zone
    }

    SECTION("A note on with velocity zero, which is a note off")
    {
        midiNoteOn(*eng, 0, PLAY_KEY, 30);
        REQUIRE(liveVoices(part, 0) == 0);

        midiNoteOn(*eng, 0, PLAY_KEY, 0);
        REQUIRE(liveVoicesInZone(part, 0, 0) == 1); // 30 is in the soft zone
    }

    SECTION("All notes off leaves nothing to release")
    {
        midiNoteOn(*eng, 0, PLAY_KEY, 100);
        midiCC(*eng, 0, 123, 0);
        midiNoteOff(*eng, 0, PLAY_KEY);
        REQUIRE(liveVoices(part, 0) == 0);
    }
}

TEST_CASE("Two CLAP notes on one key release one at a time", "[releasetrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupGroups(*eng, 1);
    setVoiceCreation(part, 0, VCM::ON_NOTE_OFF);

    eng->processNoteOnEvent(0, 0, PLAY_KEY, 100, 0.5f, 0.f);
    eng->processNoteOnEvent(0, 0, PLAY_KEY, 101, 0.5f, 0.f);
    REQUIRE(liveVoices(part, 0) == 0);

    eng->processNoteOffEvent(0, 0, PLAY_KEY, 100, 0.f);
    REQUIRE(liveVoices(part, 0) == 1);

    eng->processNoteOffEvent(0, 0, PLAY_KEY, 101, 0.f);
    REQUIRE(liveVoices(part, 0) == 2);
}

TEST_CASE("Trigger conditions still gate a release group", "[releasetrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupGroups(*eng, 1);
    setVoiceCreation(part, 0, VCM::ON_NOTE_OFF);

    auto &tc = part.getGroup(0)->triggerConditions;
    tc.storage[0].id = scxt::engine::GroupTriggerID::PROGRAM_CHANGE;
    tc.storage[0].args[0] = 3;
    tc.storage[0].args[1] = 5;
    tc.active[0] = true;
    tc.setupOnUnstream(part.groupTriggerInstrumentState);

    // program 0 is outside 3..5, so the release finds nothing to play
    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);
    REQUIRE(liveVoices(part, 0) == 0);

    uint8_t pc[3]{0xc0, 4, 0};
    eng->processMIDI1Event(0, pc);

    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);
    REQUIRE(liveVoices(part, 0) == 1);
}

TEST_CASE("A keyswitch still picks which release group sounds", "[releasetrigger]")
{
    static constexpr int SW_A{24}, SW_B{25};

    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupGroups(*eng, 2);

    for (auto [gi, key] : {std::make_pair(0, SW_A), std::make_pair(1, SW_B)})
    {
        setVoiceCreation(part, gi, VCM::ON_NOTE_OFF);
        auto &tc = part.getGroup(gi)->triggerConditions;
        tc.storage[0].id = scxt::engine::GroupTriggerID::KEYSWITCH_LATCH;
        tc.storage[0].args[0] = key;
        tc.active[0] = true;
        tc.setupOnUnstream(part.groupTriggerInstrumentState);
    }
    part.guaranteeKeyswitchLatchCoherence(*eng);

    // switch to B, then play and release
    eng->processNoteOnEvent(0, 0, SW_B, -1, 1.f, 0.f);
    eng->processNoteOffEvent(0, 0, SW_B, -1, 0.f);
    REQUIRE(liveVoices(part, 0) == 0);
    REQUIRE(liveVoices(part, 1) == 0);

    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);
    REQUIRE(liveVoices(part, 0) == 0);
    REQUIRE(liveVoices(part, 1) == 1);
}

TEST_CASE("Voice creation mode streams", "[releasetrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupGroups(*eng, 2);
    setVoiceCreation(part, 1, VCM::ON_NOTE_OFF);

    auto json = scxt::json::streamEngineState(*eng);

    // These tests run on the test thread, so the stream path's thread assert needs waiving
    std::unique_ptr<scxt::engine::Engine> other(makeEngine());
    {
        auto bg = other->getMessageController()->threadingChecker.bypassChecksInScope();
        scxt::json::unstreamEngineState(*other, json);
    }

    auto &opart = *other->getPatch()->getPart(0);
    REQUIRE(opart.getGroup(0)->triggerConditions.voiceCreationMode == VCM::ON_NOTE_ON);
    REQUIRE(opart.getGroup(1)->triggerConditions.voiceCreationMode == VCM::ON_NOTE_OFF);
    REQUIRE(opart.getGroup(1)->triggerConditions.createsVoicesOnRelease());
}

TEST_CASE("The release gate substitution only rewrites gate-following modes", "[releasetrigger]")
{
    /*
     * The rule itself, in isolation: a release voice has no gate, so GATED and SEMI_GATED are
     * rewritten - the AEG to sample gated, everything else to one shot. Modes somebody chose
     * deliberately because they ignore the gate are left exactly as they were.
     */
    using gm_t = mm::AdsrStorage::GateMode;
    using sub_t = scxt::modulation::shared::ReleaseGateSubstitution;
    using stage_t = scxt::voice::Voice::ahdsrenv_t;

    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupGroups(*eng, 1);
    eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    auto *v = firstVoiceIn(part, 0);
    REQUIRE(v != nullptr);

    mm::AdsrStorage adsr;
    // keyGate false and a mid-shape stage, so each mode gives a distinguishable answer
    auto gateFor = [&](gm_t mode, sub_t sub, bool samplePlaying) {
        adsr.gateMode = mode;
        return v->getEnvSpecificGate(false, adsr, stage_t::s_decay, samplePlaying, sub);
    };

    SECTION("With no substitution the stored mode stands")
    {
        REQUIRE(gateFor(gm_t::GATED, sub_t::NONE, true) == false); // follows the absent key
        REQUIRE(gateFor(gm_t::SEMI_GATED, sub_t::NONE, true) == false);
    }

    SECTION("The AEG is rewritten to follow the sample")
    {
        REQUIRE(gateFor(gm_t::GATED, sub_t::SAMPLE_GATED, true) == true);
        REQUIRE(gateFor(gm_t::GATED, sub_t::SAMPLE_GATED, false) == false);
        REQUIRE(gateFor(gm_t::SEMI_GATED, sub_t::SAMPLE_GATED, true) == true);
    }

    SECTION("Other EGs are rewritten to one shot, which reads the stage not the sample")
    {
        REQUIRE(gateFor(gm_t::GATED, sub_t::ONE_SHOT, false) == true); // s_decay < s_sustain
        REQUIRE(gateFor(gm_t::SEMI_GATED, sub_t::ONE_SHOT, false) == true);
    }

    SECTION("Gate independent modes are left alone by either substitution")
    {
        for (auto sub : {sub_t::ONE_SHOT, sub_t::SAMPLE_GATED})
        {
            REQUIRE(gateFor(gm_t::ONESHOT, sub, false) == true);       // still stage driven
            REQUIRE(gateFor(gm_t::SAMPLE_GATED, sub, false) == false); // still sample driven
            REQUIRE(gateFor(gm_t::SAMPLE_GATED, sub, true) == true);
        }
    }
}

namespace releasetrigger_test
{
/*
 * The substitution only shows up once audio runs, and a sample gated AEG needs a generator to
 * follow - a blank zone has none, so these need a real sample under the zone.
 */
struct SoundingFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Part *part{nullptr};

    SoundingFixture()
    {
        eng.reset(makeEngine());
        part = eng->getPatch()->getPart(0).get();

        auto p = samplePath("WavStereo48k.wav");
        REQUIRE(std::filesystem::exists(p));

        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        auto sid = eng->getSampleManager()->loadSampleByPath(p);
        REQUIRE(sid.has_value());

        for (int gi = 0; gi < 2; ++gi)
        {
            part->addGroup();
            addZone(*part, gi, 48, 72);
            auto &zone = part->getGroup(gi)->getZone(0);
            zone->variantData.variants[0].sampleID = *sid;
            zone->variantData.variants[0].active = true;
            REQUIRE(zone->attachToSample(*eng->getSampleManager(), 0,
                                         scxt::engine::Zone::SampleInformationRead::ENDPOINTS));

            // straight to sustain, so "did the gate hold it up" is the only question left
            auto &aeg = zone->egStorage[0];
            aeg.gateMode = mm::AdsrStorage::GateMode::GATED;
            aeg.a = 0.f;
            aeg.h = 0.f;
            aeg.d = 0.f;
            aeg.s = 1.f;
            aeg.r = 0.4f;
        }
        // group 0 sounds on the way down, group 1 on the way up
        setVoiceCreation(*part, 1, VCM::ON_NOTE_OFF);
    }

    // Cut the release group's playback short so its generator finishes in a few blocks
    void shortenReleaseZone(int64_t samples)
    {
        auto &v = part->getGroup(1)->getZone(0)->variantData.variants[0];
        v.endSample = v.startSample + samples;
    }

    void runBlocks(int n)
    {
        for (int i = 0; i < n; ++i)
            eng->processAudio();
    }
};
} // namespace releasetrigger_test

TEST_CASE("A gated AEG on a release voice follows the sample, not the key", "[releasetrigger]")
{
    using stage_t = scxt::voice::Voice::ahdsrenv_t;

    SoundingFixture f;
    f.eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    f.eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);
    f.runBlocks(20);

    auto *held = firstVoiceIn(*f.part, 0);
    auto *rel = firstVoiceIn(*f.part, 1);
    REQUIRE(held != nullptr);
    REQUIRE(rel != nullptr);
    REQUIRE(rel->createdByReleaseTrigger);
    REQUIRE(held->createdByReleaseTrigger == false);

    // both were let go by the same note off
    REQUIRE(held->isGated == false);
    REQUIRE(rel->isGated == false);

    // the ordinary voice lost its gate and is on its way out
    REQUIRE(held->aeg.stage >= stage_t::s_release);

    // the release voice is held up by its sample instead, so it is still sounding
    REQUIRE(rel->aeg.stage < stage_t::s_release);
    REQUIRE(rel->isVoicePlaying);
    REQUIRE(rel->aeg.outBlock0 > 0.f);
}

TEST_CASE("A release voice ends when its sample does", "[releasetrigger]")
{
    using stage_t = scxt::voice::Voice::ahdsrenv_t;

    SoundingFixture f;
    f.shortenReleaseZone(512); // 32 blocks at blockSize 16

    f.eng->processNoteOnEvent(0, 0, PLAY_KEY, -1, 1.f, 0.f);
    f.eng->processNoteOffEvent(0, 0, PLAY_KEY, -1, 0.f);

    auto *rel = firstVoiceIn(*f.part, 1);
    REQUIRE(rel != nullptr);

    // still going while there is sample left
    f.runBlocks(4);
    REQUIRE(rel->aeg.stage < stage_t::s_release);
    REQUIRE(rel->isVoicePlaying);

    /*
     * And gone once there isn't. The gate is read at the top of a block and the generator
     * finishes at the bottom of it, so the voice ends on the block the sample runs out rather
     * than stepping the envelope to release first - the sample is the whole length of the note.
     */
    f.runBlocks(80);
    REQUIRE(rel->isVoicePlaying == false);
}
