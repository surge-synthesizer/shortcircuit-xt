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
#include "engine/group_triggers.h"

#include "test_utils.h"

/*
 * Group trigger conditions which watch a continuous performance controller rather than a key:
 * program change and pitch bend. Like the keyswitch tests these drive the engine on the test
 * thread so real findZone logic decides who sounds.
 */

// Named, not anonymous: unity builds paste these test files into one TU, where an anonymous
// namespace would still collide with the identically named helpers in keyswitch_tests.cpp
namespace grouptrigger_test
{

static constexpr int PLAY_KEY{60};

// One group covering 48-72 with a single range condition on it
static scxt::engine::Part &setupOneConditionGroup(scxt::engine::Engine &eng,
                                                  scxt::engine::GroupTriggerID id, float lo,
                                                  float hi)
{
    auto &part = *eng.getPatch()->getPart(0);
    part.addGroup();
    addBlankZoneToGroup(part, 0, 48, 72);

    auto &tc = part.getGroup(0)->triggerConditions;
    tc.storage[0].id = id;
    tc.storage[0].args[0] = lo;
    tc.storage[0].args[1] = hi;
    tc.active[0] = true;
    tc.setupOnUnstream(part.groupTriggerInstrumentState);
    return part;
}

static void sendProgramChange(scxt::engine::Engine &eng, int channel, int program)
{
    uint8_t data[3]{(uint8_t)(0xc0 | channel), (uint8_t)program, 0};
    eng.processMIDI1Event(0, data);
}

// bend is signed 14 bit, -8192 .. 8191, centered at 0
static void sendPitchBend(scxt::engine::Engine &eng, int channel, int bend)
{
    auto bv = std::clamp(bend + 8192, 0, 16383);
    uint8_t data[3]{(uint8_t)(0xe0 | channel), (uint8_t)(bv & 0x7f), (uint8_t)((bv >> 7) & 0x7f)};
    eng.processMIDI1Event(0, data);
}

/*
 * Play PLAY_KEY, report how many voices it made, then let go. These tests never run audio, so
 * the released voices of earlier presses stay assigned and ringing forever - counting only the
 * gated ones is what keeps one press from being seen by the next.
 */
static int playAndCount(scxt::engine::Engine &eng, int channel = 0)
{
    eng.processNoteOnEvent(0, channel, PLAY_KEY, -1, 1.f, 0.f);

    int n{0};
    for (const auto &zone : eng.getPatch()->getPart(0)->getGroup(0)->getZones())
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            const auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned && v->isGated && v->key == PLAY_KEY)
                n++;
        }

    eng.processNoteOffEvent(0, channel, PLAY_KEY, -1, 0.f);
    return n;
}

TEST_CASE("Program change trigger - the default program is zero", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupOneConditionGroup(*eng, scxt::engine::GroupTriggerID::PROGRAM_CHANGE, 0, 0);

    // No program change has ever arrived, so a group watching for program 0 sounds
    REQUIRE(part.lastProgramChange == 0);
    REQUIRE(playAndCount(*eng) >= 1);
}

TEST_CASE("Program change trigger - sounds only inside the range", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupOneConditionGroup(*eng, scxt::engine::GroupTriggerID::PROGRAM_CHANGE, 3, 5);

    // Program 0 is outside 3..5
    REQUIRE(playAndCount(*eng) == 0);

    sendProgramChange(*eng, 0, 4);
    REQUIRE(playAndCount(*eng) >= 1);

    // Both endpoints are in
    sendProgramChange(*eng, 0, 3);
    REQUIRE(playAndCount(*eng) >= 1);
    sendProgramChange(*eng, 0, 5);
    REQUIRE(playAndCount(*eng) >= 1);

    sendProgramChange(*eng, 0, 6);
    REQUIRE(playAndCount(*eng) == 0);
}

TEST_CASE("Program change trigger - bounds given backwards still describe a range",
          "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupOneConditionGroup(*eng, scxt::engine::GroupTriggerID::PROGRAM_CHANGE, 5, 3);

    sendProgramChange(*eng, 0, 4);
    REQUIRE(playAndCount(*eng) >= 1);
    sendProgramChange(*eng, 0, 7);
    REQUIRE(playAndCount(*eng) == 0);
}

TEST_CASE("Program change trigger - a part ignores other channels", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupOneConditionGroup(*eng, scxt::engine::GroupTriggerID::PROGRAM_CHANGE, 3, 5);
    part.configuration.channel = 2;

    sendProgramChange(*eng, 7, 4);
    REQUIRE(part.lastProgramChange == 0);

    sendProgramChange(*eng, 2, 4);
    REQUIRE(part.lastProgramChange == 4);
    REQUIRE(playAndCount(*eng, 2) >= 1);
}

TEST_CASE("Pitch bend trigger - sounds only inside the range", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupOneConditionGroup(*eng, scxt::engine::GroupTriggerID::PITCH_BEND, 4000, 8191);

    // Wheel centered
    REQUIRE(playAndCount(*eng) == 0);

    sendPitchBend(*eng, 0, 6000);
    REQUIRE(playAndCount(*eng) >= 1);

    // The endpoints are in, including the top of the 14 bit range
    sendPitchBend(*eng, 0, 4000);
    REQUIRE(playAndCount(*eng) >= 1);
    sendPitchBend(*eng, 0, 8191);
    REQUIRE(playAndCount(*eng) >= 1);

    sendPitchBend(*eng, 0, 3999);
    REQUIRE(playAndCount(*eng) == 0);

    sendPitchBend(*eng, 0, -6000);
    REQUIRE(playAndCount(*eng) == 0);
}

TEST_CASE("Pitch bend trigger - a negative range works", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    setupOneConditionGroup(*eng, scxt::engine::GroupTriggerID::PITCH_BEND, -8192, -4000);

    REQUIRE(playAndCount(*eng) == 0);
    sendPitchBend(*eng, 0, -6500);
    REQUIRE(playAndCount(*eng) >= 1);

    // Wheel all the way down is -8192, not -8191
    sendPitchBend(*eng, 0, -8192);
    REQUIRE(eng->getPatch()->getPart(0)->pitchBend14Bit == -8192);
    REQUIRE(playAndCount(*eng) >= 1);
}

TEST_CASE("Pitch bend trigger - reads the gesture, not the smoothed DSP value", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = setupOneConditionGroup(*eng, scxt::engine::GroupTriggerID::PITCH_BEND, 4000, 8191);

    /*
     * A part with nothing sounding never runs its lag, so pitchBendValue is still centered here.
     * The first note after a bend has to see the bend anyway.
     */
    sendPitchBend(*eng, 0, 6000);
    REQUIRE(part.pitchBendValue == Approx(0.f).margin(1e-6));
    REQUIRE(part.pitchBend14Bit == 6000);
    REQUIRE(playAndCount(*eng) >= 1);
}

TEST_CASE("Group trigger ids for program change and pitch bend round trip as strings",
          "[grouptrigger]")
{
    for (auto id :
         {scxt::engine::GroupTriggerID::PROGRAM_CHANGE, scxt::engine::GroupTriggerID::PITCH_BEND})
    {
        auto s = scxt::engine::toStringGroupTriggerID(id);
        REQUIRE(s != "n");
        REQUIRE(scxt::engine::fromStringGroupTriggerID(s) == id);
        REQUIRE(scxt::engine::getGroupTriggerDisplayName(id) != "ERROR");
    }
}

/*
 * Round robin. These need per-group answers rather than a voice count, so they play a key and
 * report which groups sounded as a bitmask over group index - bit 0 is group 0.
 */
static uint32_t addRoundRobinGroup(scxt::engine::Part &part, scxt::engine::GroupTriggerID kind,
                                   int set, int ordinal, int keyLo = 48, int keyHi = 72)
{
    auto gidx = (int)part.addGroup() - 1;
    addBlankZoneToGroup(part, gidx, keyLo, keyHi);

    auto &tc = part.getGroup(gidx)->triggerConditions;
    tc.storage[0].id = kind;
    tc.storage[0].args[0] = (float)set;
    tc.storage[0].args[1] = (float)ordinal;
    tc.active[0] = true;
    tc.setupOnUnstream(part.groupTriggerInstrumentState);
    return 1u << gidx;
}

// Same "count only gated voices" trick as playAndCount - see the note there
static uint32_t playAndSoundingGroups(scxt::engine::Engine &eng, int key = PLAY_KEY,
                                      int channel = 0)
{
    eng.processNoteOnEvent(0, channel, key, -1, 1.f, 0.f);

    uint32_t res{0};
    auto &part = *eng.getPatch()->getPart(0);
    for (int g = 0; g < (int)part.getGroups().size(); ++g)
        for (const auto &zone : part.getGroup(g)->getZones())
            for (int i = 0; i < (int)scxt::maxVoices; ++i)
            {
                const auto *v = zone->voiceWeakPointers[i];
                if (v && v->isVoiceAssigned && v->isGated && v->key == key)
                    res |= (1u << g);
            }

    eng.processNoteOffEvent(0, channel, key, -1, 0.f);
    return res;
}

TEST_CASE("Round robin cycle - two groups alternate", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    auto g0 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1);
    auto g1 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 2);

    // Both groups cover the key, so this also shows one note spends exactly one slot
    REQUIRE(playAndSoundingGroups(*eng) == g0);
    REQUIRE(playAndSoundingGroups(*eng) == g1);
    REQUIRE(playAndSoundingGroups(*eng) == g0);
    REQUIRE(playAndSoundingGroups(*eng) == g1);
}

TEST_CASE("Round robin cycle - a split keyboard alternates with silence", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    auto g0 =
        addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1, 48, 60);
    auto g1 =
        addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 2, 61, 72);

    /*
     * The spec's case. Key 65 is only in the second group, but the counter belongs to the set
     * rather than to the note, so it still advances and the press lands on group 0's turn.
     */
    REQUIRE(playAndSoundingGroups(*eng, 65) == 0);
    REQUIRE(playAndSoundingGroups(*eng, 65) == g1);
    REQUIRE(playAndSoundingGroups(*eng, 65) == 0);
    REQUIRE(playAndSoundingGroups(*eng, 65) == g1);

    // And the same the other way up the keyboard
    REQUIRE(playAndSoundingGroups(*eng, 55) == g0);
    REQUIRE(playAndSoundingGroups(*eng, 55) == 0);
}

TEST_CASE("Round robin cycle - the first press plays the lowest ordinal", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    auto g0 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 7);
    auto g1 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 3);

    REQUIRE(playAndSoundingGroups(*eng) == g1);
    REQUIRE(playAndSoundingGroups(*eng) == g0);
    REQUIRE(playAndSoundingGroups(*eng) == g1);
}

TEST_CASE("Round robin cycle - unassigned ordinals are not gaps", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    // 1, 2, 4, 9 is a four press cycle, not a nine press one with silence at 3, 5, 6, 7, 8
    std::vector<uint32_t> gs;
    for (auto ord : {1, 2, 4, 9})
        gs.push_back(
            addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, ord));

    for (int pass = 0; pass < 3; ++pass)
        for (auto g : gs)
            REQUIRE(playAndSoundingGroups(*eng) == g);
}

TEST_CASE("Round robin cycle - groups sharing an ordinal stack", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    auto g0 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1);
    auto g1 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1);
    auto g2 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 2);

    REQUIRE(playAndSoundingGroups(*eng) == (g0 | g1));
    REQUIRE(playAndSoundingGroups(*eng) == g2);
    REQUIRE(playAndSoundingGroups(*eng) == (g0 | g1));
}

TEST_CASE("Round robin cycle - a note outside the set does not advance it", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    auto g0 =
        addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1, 48, 60);
    addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 2, 48, 60);

    // Nothing maps key 30, so these presses must not eat the first slot of the cycle
    REQUIRE(playAndSoundingGroups(*eng, 30) == 0);
    REQUIRE(playAndSoundingGroups(*eng, 30) == 0);
    REQUIRE(playAndSoundingGroups(*eng, 55) == g0);
}

TEST_CASE("Round robin cycle - two sets run independently", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    auto a0 =
        addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1, 48, 60);
    auto a1 =
        addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 2, 48, 60);
    auto b0 =
        addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 1, 1, 61, 72);
    auto b1 =
        addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 1, 2, 61, 72);

    // Working set 0 leaves set 1 sitting at its start
    REQUIRE(playAndSoundingGroups(*eng, 55) == a0);
    REQUIRE(playAndSoundingGroups(*eng, 55) == a1);
    REQUIRE(playAndSoundingGroups(*eng, 55) == a0);

    REQUIRE(playAndSoundingGroups(*eng, 65) == b0);
    REQUIRE(playAndSoundingGroups(*eng, 65) == b1);

    // ... and set 0 carries on from where it was
    REQUIRE(playAndSoundingGroups(*eng, 55) == a1);
}

TEST_CASE("Round robin cycle - a note in two sets advances both once", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    auto a0 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1);
    auto a1 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 2);
    auto b0 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 1, 1);
    auto b1 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 1, 2);

    REQUIRE(playAndSoundingGroups(*eng) == (a0 | b0));
    REQUIRE(playAndSoundingGroups(*eng) == (a1 | b1));
    REQUIRE(playAndSoundingGroups(*eng) == (a0 | b0));
}

TEST_CASE("Round robin cycle - a condition that fails spends no slot", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    auto g0 =
        addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1, 48, 60);
    addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 2, 61, 72);

    // Gate the first group behind CC 20, which starts at zero
    auto &tc = part.getGroup(0)->triggerConditions;
    tc.storage[1].id =
        (scxt::engine::GroupTriggerID)((int)scxt::engine::GroupTriggerID::MIDICC + 20);
    tc.storage[1].args[0] = 64;
    tc.storage[1].args[1] = 127;
    tc.active[1] = true;
    tc.setupOnUnstream(part.groupTriggerInstrumentState);

    // Key 55 only reaches the gated group, so a press while it is shut changes nothing
    REQUIRE(playAndSoundingGroups(*eng, 55) == 0);
    REQUIRE(playAndSoundingGroups(*eng, 55) == 0);

    uint8_t cc[3]{0xb0, 20, 100};
    eng->processMIDI1Event(0, cc);
    REQUIRE(playAndSoundingGroups(*eng, 55) == g0);
}

TEST_CASE("Round robin cycle - a keyswitch press spends no slot", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    // An articulation latched to key 50, which is inside the round robin groups' own range
    part.addGroup();
    addBlankZoneToGroup(part, 0, 48, 72);
    auto &ktc = part.getGroup(0)->triggerConditions;
    ktc.storage[0].id = scxt::engine::GroupTriggerID::KEYSWITCH_LATCH;
    ktc.storage[0].args[0] = 50;
    ktc.active[0] = true;
    ktc.setupOnUnstream(part.groupTriggerInstrumentState);
    part.guaranteeKeyswitchLatchCoherence(*eng);

    auto g1 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1);
    auto g2 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 2);

    // The switch key is consumed by the switch, so it must not turn the cycle over
    REQUIRE(playAndSoundingGroups(*eng, 50) == 0);
    REQUIRE(playAndSoundingGroups(*eng, 50) == 0);

    REQUIRE((playAndSoundingGroups(*eng, 60) & (g1 | g2)) == g1);
    REQUIRE((playAndSoundingGroups(*eng, 60) & (g1 | g2)) == g2);
}

TEST_CASE("Round robin cycle - editing the live ordinal away hands on to the next",
          "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    auto g0 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1);
    auto g1 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 2);
    auto g2 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 3);

    REQUIRE(playAndSoundingGroups(*eng) == g0); // sitting on ordinal 1

    // Move ordinal 1 out from under the live position. The next press moves up, it does not stall
    part.getGroup(0)->triggerConditions.storage[0].args[1] = 5;
    part.getGroup(0)->triggerConditions.setupOnUnstream(part.groupTriggerInstrumentState);

    REQUIRE(playAndSoundingGroups(*eng) == g1); // 2
    REQUIRE(playAndSoundingGroups(*eng) == g2); // 3
    REQUIRE(playAndSoundingGroups(*eng) == g0); // 5, the new top of the cycle
    REQUIRE(playAndSoundingGroups(*eng) == g1); // wraps back to 2
}

TEST_CASE("Round robin random - draws from the whole set", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    eng->rng.reseed(8675309);

    std::vector<uint32_t> gs;
    for (int i = 0; i < 4; ++i)
        gs.push_back(
            addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_RANDOM, 0, 0));

    uint32_t everSounded{0};
    for (int i = 0; i < 200; ++i)
    {
        auto s = playAndSoundingGroups(*eng);
        // Exactly one group at a time - random and shuffle never stack
        REQUIRE(std::popcount(s) == 1);
        everSounded |= s;
    }

    for (auto g : gs)
        REQUIRE((everSounded & g) == g);
}

TEST_CASE("Round robin random - repeats, unlike shuffle", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    eng->rng.reseed(24601);

    for (int i = 0; i < 3; ++i)
        addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_RANDOM, 0, 0);

    bool sawRepeat{false};
    uint32_t prev{0};
    for (int i = 0; i < 100 && !sawRepeat; ++i)
    {
        auto s = playAndSoundingGroups(*eng);
        sawRepeat = (s == prev);
        prev = s;
    }
    REQUIRE(sawRepeat);
}

TEST_CASE("Round robin shuffle - every pass is a permutation of the set", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    eng->rng.reseed(112358);

    constexpr int members{4};
    uint32_t all{0};
    for (int i = 0; i < members; ++i)
        all |= addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_SHUFFLE, 0, 0);

    // Each member wins exactly once per pass, and the passes keep coming
    bool sawADifferentOrder{false};
    uint32_t firstPass{0};
    for (int pass = 0; pass < 8; ++pass)
    {
        uint32_t seen{0};
        uint32_t order{0};
        for (int i = 0; i < members; ++i)
        {
            auto s = playAndSoundingGroups(*eng);
            REQUIRE(std::popcount(s) == 1);
            REQUIRE((seen & s) == 0); // nobody twice in a pass
            seen |= s;
            order = order * 16 + (uint32_t)std::countr_zero(s);
        }
        REQUIRE(seen == all);

        if (pass == 0)
            firstPass = order;
        else if (order != firstPass)
            sawADifferentOrder = true;
    }
    REQUIRE(sawADifferentOrder);
}

TEST_CASE("Round robin - the same number in different kinds is a different set", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    // RR1 and RN1 are two unrelated round robins that happen to share a number
    auto c0 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1);
    auto c1 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 2);
    auto r0 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_RANDOM, 0, 0);
    auto r1 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_RANDOM, 0, 0);

    // The cycle keeps its own count while the random pair picks one of themselves each press
    for (int i = 0; i < 6; ++i)
    {
        auto s = playAndSoundingGroups(*eng);
        REQUIRE((s & (c0 | c1)) == (i % 2 == 0 ? c0 : c1));
        REQUIRE(std::popcount(s & (r0 | r1)) == 1);
    }
}

TEST_CASE("Round robin - the live position is not part of the instrument", "[grouptrigger]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);

    auto g0 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 1);
    auto g1 = addRoundRobinGroup(part, scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE, 0, 2);

    REQUIRE(playAndSoundingGroups(*eng) == g0);

    // Whatever a performance was in the middle of, a load starts the cycle over
    part.setupOnUnstream(*eng);
    REQUIRE(playAndSoundingGroups(*eng) == g0);
    REQUIRE(playAndSoundingGroups(*eng) == g1);
}

TEST_CASE("Round robin group trigger ids round trip as strings", "[grouptrigger]")
{
    for (auto id : {scxt::engine::GroupTriggerID::ROUND_ROBIN_CYCLE,
                    scxt::engine::GroupTriggerID::ROUND_ROBIN_RANDOM,
                    scxt::engine::GroupTriggerID::ROUND_ROBIN_SHUFFLE})
    {
        auto s = scxt::engine::toStringGroupTriggerID(id);
        REQUIRE(s != "n");
        REQUIRE(scxt::engine::fromStringGroupTriggerID(s) == id);
        REQUIRE(scxt::engine::getGroupTriggerDisplayName(id) != "ERROR");
        REQUIRE(scxt::engine::isRoundRobinTriggerID(id));
    }
}

} // namespace grouptrigger_test
