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

/*
 * Velocity ranges run 1..127, since a MIDI 1 note on can't carry velocity 0. CLAP, VST3
 * and MIDI 2 can, so a float velocity rounds onto 1..127 and anything below 1.5/127 plays
 * as 1. Sessions from before stored ranges starting at 0 and load clamped to 1. See #2778.
 */

#include "catch2/catch2.hpp"

#include <memory>
#include <string>

#include <tao/json/from_string.hpp>
#include <tao/json/to_string.hpp>
#include <tao/json/value.hpp>

#include "configuration.h"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "engine/drop_mapping.h"
#include "json/engine_traits.h"
#include "json/stream.h"
#include "messaging/messaging.h"

#include "test_utils.h"

namespace velocity_floor_test
{
using Zone = scxt::engine::Zone;
using scxt::engine::KeyboardRange;
using scxt::engine::VelocityRange;

// the last version whose velocity ranges could start at 0
constexpr uint64_t legacyVersion{0x2026'10'14};

TEST_CASE("Float velocities round onto 1 to 127", "[velocity]")
{
    SECTION("every MIDI 1 velocity comes back as itself")
    {
        for (int v = 1; v <= 127; ++v)
        {
            INFO("velocity " << v);
            REQUIRE(VelocityRange::fromFloat(v / 127.0) == v);
        }
    }

    SECTION("zero and tiny velocities play as 1")
    {
        REQUIRE(VelocityRange::fromFloat(0.0) == 1);
        REQUIRE(VelocityRange::fromFloat(0.00001) == 1);
        REQUIRE(VelocityRange::fromFloat(1.4 / 127) == 1);
        REQUIRE(VelocityRange::fromFloat(-0.5) == 1);
    }

    SECTION("past 1.5/127 is 2, and the top clamps at 127")
    {
        REQUIRE(VelocityRange::fromFloat(1.6 / 127) == 2);
        REQUIRE(VelocityRange::fromFloat(1.0) == 127);
        REQUIRE(VelocityRange::fromFloat(1.5) == 127);
    }
}

TEST_CASE("Velocity ranges start at 1", "[velocity]")
{
    SECTION("a default range is 1 to 127")
    {
        VelocityRange vr;
        REQUIRE(vr.velStart == 1);
        REQUIRE(vr.velEnd == 127);
    }

    SECTION("constructing from 0 clamps to 1")
    {
        VelocityRange vr{0, 127};
        REQUIRE(vr.velStart == 1);
        REQUIRE(vr.velEnd == 127);

        VelocityRange zz{0, 0};
        REQUIRE(zz.velStart == 1);
        REQUIRE(zz.velEnd == 1);
    }

    SECTION("the editor metadata runs 1 to 127")
    {
        Zone::ZoneMappingData m;
        const auto &md = scxt::datamodel::describeValue(m, m.velocityRange.velStart);
        REQUIRE(md.minVal == 1.f);
        REQUIRE(md.maxVal == 127.f);
    }
}

TEST_CASE("Velocity edits stop at 1", "[velocity][bound_edit]")
{
    KeyboardRange kr{48, 72};

    SECTION("a typed start of 0 is refused, 1 applies")
    {
        VelocityRange vr{10, 100};
        REQUIRE(!Zone::canApplyAbsoluteBoundEdit(Zone::VEL_RANGE_START, 0, 0, kr, vr));
        REQUIRE(Zone::canApplyAbsoluteBoundEdit(Zone::VEL_RANGE_START, 0, 1, kr, vr));
        Zone::applyAbsoluteBoundEdit(Zone::VEL_RANGE_START, 0, 1, kr, vr);
        REQUIRE(vr.velStart == 1);
    }

    SECTION("dragging the start or the whole zone below 1 is refused")
    {
        VelocityRange vr{1, 100};
        REQUIRE(!Zone::canApplyChange(Zone::VEL_RANGE_START, 0, -1, kr, vr));
        REQUIRE(!Zone::canApplyChange(Zone::MOVE_CTR_NO_ROOTKEY, 0, -1, kr, vr));
        REQUIRE(Zone::canApplyChange(Zone::MOVE_CTR_NO_ROOTKEY, 0, 1, kr, vr));
    }
}

TEST_CASE("Dropped samples split velocity from 1", "[velocity][drop]")
{
    scxt::engine::DropGeometry g;
    g.nElements = 4;
    g.key = 60;
    g.shift = true;
    auto r = scxt::engine::dropRangesFor(g);
    REQUIRE(r.size() == 4);
    REQUIRE(r.front().velLo == 1);
    REQUIRE(r.back().velHi == 127);
}

struct Fixture
{
    std::unique_ptr<scxt::engine::Engine> eng;

    Fixture()
    {
        eng.reset(makeEngine());
        part().addGroup();
    }

    scxt::engine::Part &part() { return *eng->getPatch()->getPart(0); }
    scxt::engine::Group &group() { return *part().getGroup(0); }
    Zone &zone(int i = 0) { return *group().getZone(i); }

    void addZone(int velLo, int velHi)
    {
        auto z = std::make_unique<Zone>();
        z->mapping.keyboardRange = {48, 72};
        z->mapping.velocityRange = {velLo, velHi};
        z->mapping.rootKey = 60;
        z->initialize();
        group().addZone(z);
    }

    // live voices in one zone after a note on at velocity
    int voicesAt(double velocity, int zoneIndex)
    {
        eng->processNoteOnEvent(0, 0, 60, -1, velocity, 0.f);
        int n{0};
        for (auto *v : zone(zoneIndex).voiceWeakPointers)
            if (v && v->isVoiceAssigned && v->terminationSequence < 0)
                n++;
        eng->processNoteOffEvent(0, 0, 60, -1, 0.f);
        for (int i = 0; i < 64; ++i)
            eng->processAudio();
        return n;
    }
};

TEST_CASE("A tiny float velocity plays as velocity 1", "[velocity]")
{
    Fixture f;
    f.addZone(1, 1);
    f.addZone(2, 127);

    for (auto vel : {0.0, 0.00001, 1.0 / 127, 1.4 / 127})
    {
        INFO("velocity " << vel);
        REQUIRE(f.voicesAt(vel, 0) == 1);
        REQUIRE(f.voicesAt(vel, 1) == 0);
    }

    REQUIRE(f.voicesAt(2.0 / 127, 0) == 0);
    REQUIRE(f.voicesAt(2.0 / 127, 1) == 1);
}

TEST_CASE("Old sessions with a velocity range from 0 load from 1", "[velocity][streaming]")
{
    Fixture f;
    f.addZone(1, 127);
    f.addZone(1, 40);

    std::string current;
    {
        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_DAW);
        current = scxt::json::streamEngineState(*f.eng);
    }

    auto v = tao::json::from_string(current);
    v["streamingVersion"] = legacyVersion;
    auto &vz = v["patch"]["parts"].get_array()[0]["groups"].get_array()[0]["zones"].get_array();
    REQUIRE(vz.size() == 2);
    vz[0]["mappingData"]["velR"]["vs"] = 0;
    vz[1]["mappingData"]["velR"]["vs"] = 0;
    vz[1]["mappingData"]["velR"]["ve"] = 0;

    Fixture r;
    {
        auto bg = r.eng->getMessageController()->threadingChecker.bypassChecksInScope();
        scxt::json::unstreamEngineState(*r.eng, tao::json::to_string(v));
    }
    REQUIRE(r.group().getZones().size() == 2);
    REQUIRE(r.zone(0).mapping.velocityRange.velStart == 1);
    REQUIRE(r.zone(0).mapping.velocityRange.velEnd == 127);
    REQUIRE(r.zone(1).mapping.velocityRange.velStart == 1);
    REQUIRE(r.zone(1).mapping.velocityRange.velEnd == 1);
}

TEST_CASE("An old part file with a velocity range from 0 loads from 1", "[velocity][streaming]")
{
    Fixture f;
    f.addZone(1, 127);
    auto &eng = *f.eng;
    auto bg = eng.getMessageController()->threadingChecker.bypassChecksInScope();

    std::string part;
    {
        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_PART);
        part = tao::json::to_string(scxt::json::scxt_value(f.part()));
    }
    auto v = tao::json::from_string(part);
    v["streamingVersion"] = legacyVersion;
    v["groups"].get_array()[0]["zones"].get_array()[0]["mappingData"]["velR"]["vs"] = 0;

    scxt::json::unstreamPartState(eng, 0, tao::json::to_string(v));
    REQUIRE(f.zone(0).mapping.velocityRange.velStart == 1);
}
} // namespace velocity_floor_test
