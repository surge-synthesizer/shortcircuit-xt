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

} // namespace grouptrigger_test
