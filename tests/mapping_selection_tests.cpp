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

#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "selection/selection_manager.h"
#include "undo_manager/undo.h"

namespace cmsg = scxt::messaging::client;
using Zone = scxt::engine::Zone;
using ZMD = Zone::ZoneMappingData;

namespace
{
#define MAP_OFF(f) offsetof(ZMD, f)

struct MappingFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    MappingFixture()
    {
        th.start();
        th.stepUI();
    }

    scxt::engine::Engine &engine() { return *th.engine; }

    template <typename T> void send(const T &msg, size_t drainSteps = 30)
    {
        th.sendToSerialization(msg);
        th.stepUI(drainSteps);
    }

    void sendUndo(size_t drainSteps = 30) { send(cmsg::Undo(true), drainSteps); }
};

/*
 * Three zones side by side in one group, all selected, the middle one leading. Each starts
 * with its own mapping values so a test can tell "took the lead's value" apart from "was
 * already that" and, more importantly, can catch a field being reset that nobody edited.
 */
struct ThreeZones
{
    MappingFixture f;
    static constexpr int lead{1};

    ThreeZones()
    {
        f.send(cmsg::AddBlankZone({0, 0, 48, 59, 0, 127}));
        f.send(cmsg::AddBlankZone({0, 0, 60, 71, 0, 100}));
        f.send(cmsg::AddBlankZone({0, 0, 72, 83, 20, 127}));

        f.send(cmsg::ApplySelectActions({{0, 0, lead, true, true, true}}));
        for (int z = 0; z < 3; ++z)
            if (z != lead)
                f.send(cmsg::ApplySelectActions({{0, 0, z, true, false, false}}));

        auto &grp = f.engine().getPatch()->getPart(0)->getGroup(0);
        REQUIRE(grp->getZones().size() == 3);

        const auto &sm = f.engine().getSelectionManager();
        REQUIRE(sm->currentlySelectedZones().size() == 3);
        REQUIRE(sm->currentLeadZone(f.engine())->zone == lead);

        for (int z = 0; z < 3; ++z)
        {
            auto &m = zone(z).mapping;
            m.rootKey = (int16_t)(50 + z);
            m.pan = -0.5f + 0.5f * z;
            m.amplitude = -6.f * z;
            m.pitchOffset = 1.f * z;
            m.tracking = 0.5f + 0.25f * z;
            m.pbDown = (int16_t)(1 + z);
            m.pbUp = (int16_t)(4 + z);
        }
    }

    Zone &zone(int z) { return *f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(z); }

    // everything an edit to one scalar must leave alone on every zone
    struct Geometry
    {
        int16_t rootKey;
        scxt::engine::KeyboardRange kr;
        scxt::engine::VelocityRange vr;
    };
    Geometry geometryOf(int z)
    {
        auto &m = zone(z).mapping;
        return {m.rootKey, m.keyboardRange, m.velocityRange};
    }
    void requireGeometryUnchanged(int z, const Geometry &g)
    {
        INFO("zone " << z);
        auto &m = zone(z).mapping;
        REQUIRE(m.rootKey == g.rootKey);
        REQUIRE(m.keyboardRange.keyStart == g.kr.keyStart);
        REQUIRE(m.keyboardRange.keyEnd == g.kr.keyEnd);
        REQUIRE(m.keyboardRange.fadeStart == g.kr.fadeStart);
        REQUIRE(m.keyboardRange.fadeEnd == g.kr.fadeEnd);
        REQUIRE(m.velocityRange.velStart == g.vr.velStart);
        REQUIRE(m.velocityRange.velEnd == g.vr.velEnd);
        REQUIRE(m.velocityRange.fadeStart == g.vr.fadeStart);
        REQUIRE(m.velocityRange.fadeEnd == g.vr.fadeEnd);
    }
};
} // namespace

TEST_CASE("Mapping fields know whether they cross a selection", "[mapping]")
{
    SECTION("the settings which span the selection")
    {
        for (auto o : {MAP_OFF(pbDown), MAP_OFF(pbUp), MAP_OFF(amplitude), MAP_OFF(pan),
                       MAP_OFF(pitchOffset), MAP_OFF(tracking)})
        {
            INFO("offset " << o);
            REQUIRE(Zone::mappingFieldCrossesZones((ptrdiff_t)o, sizeof(float)));
        }
    }

    SECTION("the key center stays on the lead")
    {
        REQUIRE(!Zone::mappingFieldCrossesZones(MAP_OFF(rootKey), sizeof(int16_t)));
    }

    SECTION("every field inside the two ranges stays on the lead")
    {
        // the interior fields are the ones a plain offset equality test would miss
        const auto kr = MAP_OFF(keyboardRange);
        const auto vr = MAP_OFF(velocityRange);
        for (auto o : {kr + offsetof(scxt::engine::KeyboardRange, keyStart),
                       kr + offsetof(scxt::engine::KeyboardRange, keyEnd),
                       kr + offsetof(scxt::engine::KeyboardRange, fadeStart),
                       kr + offsetof(scxt::engine::KeyboardRange, fadeEnd),
                       vr + offsetof(scxt::engine::VelocityRange, velStart),
                       vr + offsetof(scxt::engine::VelocityRange, velEnd),
                       vr + offsetof(scxt::engine::VelocityRange, fadeStart),
                       vr + offsetof(scxt::engine::VelocityRange, fadeEnd)})
        {
            INFO("offset " << o);
            REQUIRE(!Zone::mappingFieldCrossesZones((ptrdiff_t)o, sizeof(int16_t)));
        }
    }

    SECTION("the excluded spans cover the ranges whole")
    {
        for (auto o = MAP_OFF(keyboardRange);
             o < MAP_OFF(keyboardRange) + sizeof(scxt::engine::KeyboardRange); ++o)
        {
            INFO("offset " << o);
            REQUIRE(!Zone::mappingFieldCrossesZones((ptrdiff_t)o, 1));
        }
        for (auto o = MAP_OFF(velocityRange);
             o < MAP_OFF(velocityRange) + sizeof(scxt::engine::VelocityRange); ++o)
        {
            INFO("offset " << o);
            REQUIRE(!Zone::mappingFieldCrossesZones((ptrdiff_t)o, 1));
        }
    }
}

TEST_CASE("A pan edit spans the selection without resetting anything else", "[mapping]")
{
    ThreeZones t;

    std::array<ThreeZones::Geometry, 3> geo{t.geometryOf(0), t.geometryOf(1), t.geometryOf(2)};
    std::array<float, 3> amps{t.zone(0).mapping.amplitude, t.zone(1).mapping.amplitude,
                              t.zone(2).mapping.amplitude};

    t.f.send(cmsg::UpdateZoneMappingFloatValue({MAP_OFF(pan), 0.35f}));

    for (int z = 0; z < 3; ++z)
    {
        INFO("zone " << z);
        REQUIRE(t.zone(z).mapping.pan == Approx(0.35f));
        // the neighbouring scalar is not dragged along
        REQUIRE(t.zone(z).mapping.amplitude == Approx(amps[z]));
        // and neither is the geometry, which is the whole point
        t.requireGeometryUnchanged(z, geo[z]);
    }
}

TEST_CASE("Level, pitch and keytrack span the selection", "[mapping]")
{
    ThreeZones t;

    t.f.send(cmsg::UpdateZoneMappingFloatValue({MAP_OFF(amplitude), -3.5f}));
    t.f.send(cmsg::UpdateZoneMappingFloatValue({MAP_OFF(pitchOffset), 7.25f}));
    t.f.send(cmsg::UpdateZoneMappingFloatValue({MAP_OFF(tracking), 0.75f}));

    for (int z = 0; z < 3; ++z)
    {
        INFO("zone " << z);
        REQUIRE(t.zone(z).mapping.amplitude == Approx(-3.5f));
        REQUIRE(t.zone(z).mapping.pitchOffset == Approx(7.25f));
        REQUIRE(t.zone(z).mapping.tracking == Approx(0.75f));
    }
}

TEST_CASE("The pitch bend range spans the selection", "[mapping]")
{
    ThreeZones t;

    t.f.send(cmsg::UpdateZoneMappingInt16TValue({MAP_OFF(pbDown), (int16_t)12}));
    t.f.send(cmsg::UpdateZoneMappingInt16TValue({MAP_OFF(pbUp), (int16_t)2}));

    for (int z = 0; z < 3; ++z)
    {
        INFO("zone " << z);
        REQUIRE(t.zone(z).mapping.pbDown == 12);
        REQUIRE(t.zone(z).mapping.pbUp == 2);
    }
}

TEST_CASE("The key center lands on the lead zone only", "[mapping]")
{
    ThreeZones t;

    auto other0 = t.zone(0).mapping.rootKey;
    auto other2 = t.zone(2).mapping.rootKey;

    t.f.send(cmsg::UpdateZoneMappingInt16TValue({MAP_OFF(rootKey), (int16_t)36}));

    REQUIRE(t.zone(ThreeZones::lead).mapping.rootKey == 36);
    REQUIRE(t.zone(0).mapping.rootKey == other0);
    REQUIRE(t.zone(2).mapping.rootKey == other2);
}

TEST_CASE("A mapping edit across a selection is one undo entry", "[mapping][undo]")
{
    ThreeZones t;

    std::array<float, 3> before{t.zone(0).mapping.pan, t.zone(1).mapping.pan,
                                t.zone(2).mapping.pan};
    auto depthBefore = t.f.engine().undoManager.undoStackSize();

    t.f.send(cmsg::UpdateZoneMappingFloatValue({MAP_OFF(pan), 0.35f}));

    for (int z = 0; z < 3; ++z)
        REQUIRE(t.zone(z).mapping.pan == Approx(0.35f));
    REQUIRE(t.f.engine().undoManager.undoStackSize() == depthBefore + 1);

    // each zone comes back to its own value, not to a shared one
    t.f.sendUndo();
    for (int z = 0; z < 3; ++z)
    {
        INFO("zone " << z);
        REQUIRE(t.zone(z).mapping.pan == Approx(before[z]));
    }
    REQUIRE(t.f.engine().undoManager.undoStackSize() == depthBefore);
}

TEST_CASE("A mapping drag across a selection coalesces to one undo entry", "[mapping][undo]")
{
    ThreeZones t;

    std::array<float, 3> before{t.zone(0).mapping.pan, t.zone(1).mapping.pan,
                                t.zone(2).mapping.pan};
    auto depthBefore = t.f.engine().undoManager.undoStackSize();

    t.f.send(cmsg::BeginEdit({(int32_t)cmsg::EditSubtree::zone_mapping, true, (int32_t)-1}));
    for (int i = 0; i < 10; ++i)
        t.f.send(cmsg::UpdateZoneMappingFloatValue({MAP_OFF(pan), -0.4f + 0.08f * i}));
    t.f.send(cmsg::EndEdit(true));

    REQUIRE(t.f.engine().undoManager.undoStackSize() == depthBefore + 1);
    for (int z = 0; z < 3; ++z)
        REQUIRE(t.zone(z).mapping.pan == Approx(-0.4f + 0.08f * 9));

    t.f.sendUndo();
    for (int z = 0; z < 3; ++z)
    {
        INFO("zone " << z);
        REQUIRE(t.zone(z).mapping.pan == Approx(before[z]));
    }
}

TEST_CASE("A key range edit still refuses a gesture the selection cannot take", "[mapping]")
{
    ThreeZones t;

    // the geometry path is unchanged by this work, but it now lives next door to it: a delta
    // no selected zone may take is still refused for all of them, not applied to the ones
    // which happen to fit
    std::array<int16_t, 3> starts{}, ends{};
    for (int z = 0; z < 3; ++z)
    {
        starts[z] = t.zone(z).mapping.keyboardRange.keyStart;
        ends[z] = t.zone(z).mapping.keyboardRange.keyEnd;
    }

    // delta, not lead-only: zone 2 ends at 83, so +60 runs it off the top of the keyboard
    t.f.send(
        cmsg::ApplyZoneDelta({false, false, 0, (int)Zone::ChangeDimension::KEY_RANGE_END, 60, 0}));

    for (int z = 0; z < 3; ++z)
    {
        INFO("zone " << z);
        REQUIRE(t.zone(z).mapping.keyboardRange.keyStart == starts[z]);
        REQUIRE(t.zone(z).mapping.keyboardRange.keyEnd == ends[z]);
    }
}

// keep the file unity-safe: this must not leak into a batched neighbour
#undef MAP_OFF
