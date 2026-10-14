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
 * The pitch bend range, #2325. Up and down are each a signed semitone offset reached at full
 * bend, so the default down range is -2 and a positive down range bends up. A zone either
 * follows its group's pair or uses its own, chosen by pbUseGroup.
 *
 * Sessions from before the change stored down as a positive magnitude and marked a zone that
 * followed its group with -1 in either field. Those are built here by rewriting a stream from
 * this build into the old shape and loading it through the real engine unstream.
 */

#include "catch2/catch2.hpp"

#include <memory>
#include <string>
#include <vector>

#include <tao/json/from_string.hpp>
#include <tao/json/to_string.hpp>
#include <tao/json/value.hpp>

#include "configuration.h"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "json/engine_traits.h"
#include "json/stream.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "test_utils.h"

namespace pitch_bend_range_test
{
using Zone = scxt::engine::Zone;
using Group = scxt::engine::Group;

// the last version to write down as a magnitude and -1 for "use the group"
constexpr uint64_t legacyVersion{0x2026'10'04};
// and before this one a zone equal to its group was folded onto the group as well
constexpr uint64_t preFoldVersion{0x2025'11'01};

struct Fixture
{
    std::unique_ptr<scxt::engine::Engine> eng;

    explicit Fixture(int nZones = 1)
    {
        eng.reset(makeEngine());
        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        for (int i = 0; i < nZones; ++i)
            addBlankZoneToGroup(part, 0, 0, 127);
    }

    scxt::engine::Part &part() { return *eng->getPatch()->getPart(0); }
    Group &group() { return *part().getGroup(0); }
    Zone &zone(int i = 0) { return *group().getZone(i); }

    std::string stream()
    {
        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_DAW);
        return scxt::json::streamEngineState(*eng);
    }

    void load(const std::string &state)
    {
        auto bg = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        scxt::json::unstreamEngineState(*eng, state);
    }

    // the pitch a note at the zone's root key plays with the wheel held at bend
    float pitchAtBend(float bend, int zoneIndex = 0)
    {
        part().pitchBendValue = bend;
        auto &z = zone(zoneIndex);
        eng->processNoteOnEvent(0, 0, z.mapping.rootKey, -1, 1.f, 0.f);
        scxt::voice::Voice *v{nullptr};
        for (auto *wp : z.voiceWeakPointers)
            if (wp && wp->isVoiceAssigned)
                v = wp;
        REQUIRE(v);
        auto res = v->calculateVoicePitch() - z.mapping.rootKey;
        eng->processNoteOffEvent(0, 0, z.mapping.rootKey, -1, 0.f);
        for (int i = 0; i < 8; ++i)
            eng->processAudio();
        part().pitchBendValue = 0.f;
        return res;
    }
};

struct LegacyZone
{
    int down, up;
};

/*
 * A stream as a build before the change would have written it: integer ranges, down as a
 * magnitude, -1 on a zone for "use the group" and no use-group key.
 */
std::string legacyState(const std::string &current, uint64_t version, int groupDown, int groupUp,
                        const std::vector<LegacyZone> &zones)
{
    auto v = tao::json::from_string(current);
    v["streamingVersion"] = version;
    auto &grp = v["patch"]["parts"].get_array()[0]["groups"].get_array()[0];
    grp["outputInfo"]["pbd"] = groupDown;
    grp["outputInfo"]["pbu"] = groupUp;
    auto &vz = grp["zones"].get_array();
    REQUIRE(vz.size() == zones.size());
    for (size_t i = 0; i < zones.size(); ++i)
    {
        auto &m = vz[i]["mappingData"].get_object();
        REQUIRE(m.count("pbGrp") == 1);
        m.erase("pbGrp");
        m["pbDown"] = zones[i].down;
        m["pbUp"] = zones[i].up;
    }
    return tao::json::to_string(v);
}

TEST_CASE("Pitch bend ranges default to two semitones each way", "[pitchbend]")
{
    Zone::ZoneMappingData m;
    REQUIRE(m.pbDown == -2.f);
    REQUIRE(m.pbUp == 2.f);
    REQUIRE(m.pbUseGroup);

    Group::GroupOutputInfo oi;
    REQUIRE(oi.pbDown == -2.f);
    REQUIRE(oi.pbUp == 2.f);
}

TEST_CASE("A full bend lands on the signed range", "[pitchbend]")
{
    Fixture f;
    auto &g = f.group().outputInfo;
    auto &m = f.zone().mapping;

    SECTION("the group's range, which a new zone follows")
    {
        g.pbUp = 7.f;
        g.pbDown = -12.f;
        REQUIRE(f.pitchAtBend(1.f) == Approx(7.f).margin(1e-4));
        REQUIRE(f.pitchAtBend(-1.f) == Approx(-12.f).margin(1e-4));
        REQUIRE(f.pitchAtBend(0.5f) == Approx(3.5f).margin(1e-4));
        REQUIRE(f.pitchAtBend(-0.5f) == Approx(-6.f).margin(1e-4));
        REQUIRE(f.pitchAtBend(0.f) == Approx(0.f).margin(1e-4));
    }

    SECTION("the zone's own range ignores the group's")
    {
        g.pbUp = 7.f;
        g.pbDown = -12.f;
        m.pbUseGroup = false;
        m.pbUp = 1.5f;
        m.pbDown = -0.25f;
        REQUIRE(f.pitchAtBend(1.f) == Approx(1.5f).margin(1e-4));
        REQUIRE(f.pitchAtBend(-1.f) == Approx(-0.25f).margin(1e-4));

        m.pbUseGroup = true;
        REQUIRE(f.pitchAtBend(1.f) == Approx(7.f).margin(1e-4));
        REQUIRE(f.pitchAtBend(-1.f) == Approx(-12.f).margin(1e-4));
    }

    SECTION("a positive down range bends up and a negative up range bends down")
    {
        g.pbDown = 5.f;
        g.pbUp = -3.f;
        REQUIRE(f.pitchAtBend(-1.f) == Approx(5.f).margin(1e-4));
        REQUIRE(f.pitchAtBend(1.f) == Approx(-3.f).margin(1e-4));

        m.pbUseGroup = false;
        m.pbDown = 1.5f;
        m.pbUp = -1.5f;
        REQUIRE(f.pitchAtBend(-1.f) == Approx(1.5f).margin(1e-4));
        REQUIRE(f.pitchAtBend(1.f) == Approx(-1.5f).margin(1e-4));
    }
}

TEST_CASE("Pitch bend ranges round trip", "[pitchbend][streaming]")
{
    Fixture f(2);
    f.group().outputInfo.pbUp = 7.25f;
    f.group().outputInfo.pbDown = 3.5f;
    f.zone(0).mapping.pbUseGroup = false;
    f.zone(0).mapping.pbUp = -1.f;
    f.zone(0).mapping.pbDown = -12.75f;
    f.zone(1).mapping.pbUp = 4.f;
    f.zone(1).mapping.pbDown = -4.f;

    Fixture r(2);
    r.load(f.stream());

    REQUIRE(r.group().outputInfo.pbUp == Approx(7.25f));
    REQUIRE(r.group().outputInfo.pbDown == Approx(3.5f));
    REQUIRE(!r.zone(0).mapping.pbUseGroup);
    REQUIRE(r.zone(0).mapping.pbUp == Approx(-1.f));
    REQUIRE(r.zone(0).mapping.pbDown == Approx(-12.75f));
    REQUIRE(r.zone(1).mapping.pbUseGroup);
    REQUIRE(r.zone(1).mapping.pbUp == Approx(4.f));
    REQUIRE(r.zone(1).mapping.pbDown == Approx(-4.f));
}

TEST_CASE("Old sessions keep how they bent", "[pitchbend][streaming]")
{
    Fixture f(5);
    auto current = f.stream();

    SECTION("the group's down magnitude turns negative")
    {
        Fixture r(5);
        r.load(legacyState(current, legacyVersion, 12, 7,
                           {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}}));
        REQUIRE(r.group().outputInfo.pbDown == Approx(-12.f));
        REQUIRE(r.group().outputInfo.pbUp == Approx(7.f));
        REQUIRE(r.pitchAtBend(-1.f) == Approx(-12.f).margin(1e-4));
        REQUIRE(r.pitchAtBend(1.f) == Approx(7.f).margin(1e-4));
    }

    SECTION("each zone shape lands where it bent")
    {
        Fixture r(5);
        r.load(legacyState(current, legacyVersion, 12, 7,
                           {
                               {-1, -1}, // both on the group
                               {5, 3},   // both its own
                               {12, 7},  // its own, though equal to the group's
                               {5, -1},  // own down, group up
                               {-1, 3},  // group down, own up
                           }));
        auto &oi = r.group().outputInfo;
        REQUIRE(oi.pbDown == Approx(-12.f));
        REQUIRE(oi.pbUp == Approx(7.f));

        auto requireZone = [&r](int i, bool useGroup, float down, float up) {
            INFO("zone " << i);
            auto &m = r.zone(i).mapping;
            REQUIRE(m.pbUseGroup == useGroup);
            REQUIRE(m.pbDown == Approx(down));
            REQUIRE(m.pbUp == Approx(up));
            REQUIRE(r.pitchAtBend(-1.f, i) == Approx(down).margin(1e-4));
            REQUIRE(r.pitchAtBend(1.f, i) == Approx(up).margin(1e-4));
        };
        // a zone on the group carries the group's pair, so turning it off starts from there
        requireZone(0, true, -12.f, 7.f);
        requireZone(1, false, -5.f, 3.f);
        // after the fold version, equal stays the zone's own
        requireZone(2, false, -12.f, 7.f);
        // half on the group is pinned to the group's value for that half
        requireZone(3, false, -5.f, 7.f);
        requireZone(4, false, -12.f, 3.f);

        // and so stops following the group, where a full use-group zone does not
        oi.pbDown = -1.f;
        REQUIRE(r.pitchAtBend(-1.f, 0) == Approx(-1.f).margin(1e-4));
        REQUIRE(r.pitchAtBend(-1.f, 4) == Approx(-12.f).margin(1e-4));
    }

    SECTION("before the fold a zone equal to its group follows it")
    {
        Fixture r(5);
        r.load(legacyState(current, preFoldVersion, 12, 7,
                           {{12, 7}, {12, 3}, {5, 7}, {-1, -1}, {5, 3}}));
        auto requireZone = [&r](int i, bool useGroup, float down, float up) {
            INFO("zone " << i);
            auto &m = r.zone(i).mapping;
            REQUIRE(m.pbUseGroup == useGroup);
            REQUIRE(m.pbDown == Approx(down));
            REQUIRE(m.pbUp == Approx(up));
        };
        requireZone(0, true, -12.f, 7.f);
        requireZone(1, false, -12.f, 3.f);
        requireZone(2, false, -5.f, 7.f);
        requireZone(3, true, -12.f, 7.f);
        requireZone(4, false, -5.f, 3.f);
    }

    SECTION("a session with no ranges stored takes the old defaults")
    {
        auto v = tao::json::from_string(current);
        v["streamingVersion"] = legacyVersion;
        auto &grp = v["patch"]["parts"].get_array()[0]["groups"].get_array()[0];
        grp["outputInfo"].get_object().erase("pbd");
        grp["outputInfo"].get_object().erase("pbu");
        for (auto &z : grp["zones"].get_array())
        {
            auto &m = z["mappingData"].get_object();
            m.erase("pbDown");
            m.erase("pbUp");
            m.erase("pbGrp");
        }

        Fixture r(5);
        r.load(tao::json::to_string(v));
        REQUIRE(r.group().outputInfo.pbDown == Approx(-2.f));
        REQUIRE(r.group().outputInfo.pbUp == Approx(2.f));
        for (int i = 0; i < 5; ++i)
        {
            INFO("zone " << i);
            REQUIRE(r.zone(i).mapping.pbUseGroup);
        }
        REQUIRE(r.pitchAtBend(-1.f) == Approx(-2.f).margin(1e-4));
    }
}

TEST_CASE("An old part file keeps how it bent", "[pitchbend][streaming]")
{
    Fixture f(2);
    auto &eng = *f.eng;
    auto bg = eng.getMessageController()->threadingChecker.bypassChecksInScope();

    std::string part;
    {
        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_PART);
        part = tao::json::to_string(scxt::json::scxt_value(f.part()));
    }
    auto v = tao::json::from_string(part);
    v["streamingVersion"] = legacyVersion;
    auto &grp = v["groups"].get_array()[0];
    grp["outputInfo"]["pbd"] = 24;
    auto &vz = grp["zones"].get_array();
    vz[0]["mappingData"].get_object().erase("pbGrp");
    vz[0]["mappingData"]["pbDown"] = -1;
    vz[0]["mappingData"]["pbUp"] = -1;
    vz[1]["mappingData"].get_object().erase("pbGrp");
    vz[1]["mappingData"]["pbDown"] = 3;
    vz[1]["mappingData"]["pbUp"] = 4;

    scxt::json::unstreamPartState(eng, 0, tao::json::to_string(v));

    REQUIRE(f.group().outputInfo.pbDown == Approx(-24.f));
    REQUIRE(f.zone(0).mapping.pbUseGroup);
    REQUIRE(!f.zone(1).mapping.pbUseGroup);
    REQUIRE(f.zone(1).mapping.pbDown == Approx(-3.f));
    REQUIRE(f.zone(1).mapping.pbUp == Approx(4.f));
}
} // namespace pitch_bend_range_test
