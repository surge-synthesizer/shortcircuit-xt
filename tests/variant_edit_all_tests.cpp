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
#include <string>

#include "configuration.h"
#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "selection/selection_manager.h"
#include "undo_manager/undo.h"

#ifndef SCXT_TEST_SOURCE_DIR
#define SCXT_TEST_SOURCE_DIR ""
#endif

namespace cmsg = scxt::messaging::client;
using scxt::maxVariantsPerZone;
using scxt::minimumVariantRegionInSamples;
using Zone = scxt::engine::Zone;

namespace
{
// convenience wrapper so the cases below read like the spec
struct Region
{
    int64_t s, e;
};
Region clampTo(int64_t s, int64_t e, int64_t len)
{
    Zone::clampVariantRegionToLength(s, e, len);
    return {s, e};
}
} // namespace

TEST_CASE("Edit all clamps a region onto a shorter sample", "[variants]")
{
    SECTION("region that already fits is untouched")
    {
        auto r = clampTo(2000, 3000, 10000);
        REQUIRE(r.s == 2000);
        REQUIRE(r.e == 3000);
    }

    SECTION("region overhanging the end truncates at the sample length")
    {
        auto r = clampTo(5000, 15000, 10000);
        REQUIRE(r.s == 5000);
        REQUIRE(r.e == 10000);
    }

    SECTION("region entirely past the end falls back to the minimum at the sample end")
    {
        auto r = clampTo(12000, 15000, 10000);
        REQUIRE(r.e == 10000);
        REQUIRE(r.s == 10000 - minimumVariantRegionInSamples);
    }

    SECTION("start exactly at the end still yields the minimum region")
    {
        auto r = clampTo(10000, 12000, 10000);
        REQUIRE(r.e == 10000);
        REQUIRE(r.s == 10000 - minimumVariantRegionInSamples);
    }

    SECTION("start pushed above the target's own end backs off from that end")
    {
        // only the start propagated; the target keeps its own end of 8000
        auto r = clampTo(9000, 8000, 10000);
        REQUIRE(r.e == 8000);
        REQUIRE(r.s == 8000 - minimumVariantRegionInSamples);
    }

    SECTION("end pulled below the target's own start backs off from the new end")
    {
        auto r = clampTo(5000, 2000, 10000);
        REQUIRE(r.e == 2000);
        REQUIRE(r.s == 2000 - minimumVariantRegionInSamples);
    }

    SECTION("a sample shorter than the minimum clamps to the whole sample")
    {
        auto r = clampTo(100, 200, 8);
        REQUIRE(r.s == 0);
        REQUIRE(r.e == 8);
    }

    SECTION("a longer sample does not get the region stretched")
    {
        auto r = clampTo(1000, 2000, 100000);
        REQUIRE(r.s == 1000);
        REQUIRE(r.e == 2000);
    }

    SECTION("a region backing off the front is pushed forward instead")
    {
        auto r = clampTo(0, 4, 10000);
        REQUIRE(r.s == 0);
        REQUIRE(r.e == minimumVariantRegionInSamples);
    }
}

namespace
{
struct EditAllFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    EditAllFixture()
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

std::string testSample(const std::string &n)
{
    namespace fs = std::filesystem;
    auto p = fs::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / "next" / n;
    REQUIRE(fs::exists(p));
    return p.u8string();
}

/*
 * The real fan-out: the same core call VariantDisplay::propagateEditAll makes, so a test
 * exercises the shipped policy rather than a copy of it. Fields are addressed by offset,
 * exactly as the UI addresses the control the user just moved.
 */
#define VAR_FIELD_OFF(f) offsetof(Zone::SingleVariant, f)
#define VAR_FIELD(f) VAR_FIELD_OFF(f), sizeof(Zone::SingleVariant::f)

std::array<Zone::SingleVariant, maxVariantsPerZone> fanOut(const Zone &zone, size_t lead,
                                                           ptrdiff_t off, size_t sz)
{
    auto out = zone.variantData.variants;
    const auto leadVar = out[lead];
    for (auto i = 0U; i < maxVariantsPerZone; ++i)
    {
        if (i == lead || !out[i].active)
            continue;
        Zone::applyVariantEdit(out[i], leadVar, off, sz);
    }
    return out;
}
} // namespace

TEST_CASE("Edit all never crosses positions or identity", "[variants]")
{
    using SV = Zone::SingleVariant;

    SECTION("the lead-only offsets really are the per-sample fields")
    {
        // named here so a field rename or reorder cannot silently drop one
        std::array<size_t, 8> expect{
            VAR_FIELD_OFF(startSample), VAR_FIELD_OFF(endSample),
            VAR_FIELD_OFF(startLoop),   VAR_FIELD_OFF(endLoop),
            VAR_FIELD_OFF(loopFade),    VAR_FIELD_OFF(active),
            VAR_FIELD_OFF(sampleID),    VAR_FIELD_OFF(normalizationAmplitude)};
        for (auto o : expect)
        {
            INFO("offset " << o);
            REQUIRE(!Zone::variantFieldCrossesVariants((ptrdiff_t)o));
        }
    }

    SECTION("the settings edit-all exists for do cross")
    {
        for (auto o :
             {VAR_FIELD_OFF(pan), VAR_FIELD_OFF(pitchOffset), VAR_FIELD_OFF(amplitude),
              VAR_FIELD_OFF(playMode), VAR_FIELD_OFF(loopActive), VAR_FIELD_OFF(playReverse),
              VAR_FIELD_OFF(loopMode), VAR_FIELD_OFF(loopDirection),
              VAR_FIELD_OFF(loopCountWhenCounted), VAR_FIELD_OFF(interpolationType)})
        {
            INFO("offset " << o);
            REQUIRE(Zone::variantFieldCrossesVariants((ptrdiff_t)o));
        }
    }

    SECTION("applying a lead-only field is a no-op")
    {
        SV lead, t;
        lead.startSample = 111;
        lead.endSample = 222;
        lead.startLoop = 333;
        lead.endLoop = 444;
        lead.loopFade = 55;
        lead.normalizationAmplitude = 0.25f;
        lead.active = true;

        t.startSample = 10;
        t.endSample = 20;
        t.startLoop = 30;
        t.endLoop = 40;
        t.loopFade = 5;
        t.normalizationAmplitude = 0.9f;
        const auto before = t;

        Zone::applyVariantEdit(t, lead, VAR_FIELD(startSample));
        Zone::applyVariantEdit(t, lead, VAR_FIELD(endSample));
        Zone::applyVariantEdit(t, lead, VAR_FIELD(startLoop));
        Zone::applyVariantEdit(t, lead, VAR_FIELD(endLoop));
        Zone::applyVariantEdit(t, lead, VAR_FIELD(loopFade));
        Zone::applyVariantEdit(t, lead, VAR_FIELD(normalizationAmplitude));
        Zone::applyVariantEdit(t, lead, VAR_FIELD(active));
        Zone::applyVariantEdit(t, lead, VAR_FIELD(sampleID));

        REQUIRE(t.startSample == before.startSample);
        REQUIRE(t.endSample == before.endSample);
        REQUIRE(t.startLoop == before.startLoop);
        REQUIRE(t.endLoop == before.endLoop);
        REQUIRE(t.loopFade == before.loopFade);
        REQUIRE(t.normalizationAmplitude == Approx(before.normalizationAmplitude));
        REQUIRE(t.active == before.active);
        REQUIRE(t.sampleID == before.sampleID);
    }

    SECTION("applying a shared field copies exactly that field")
    {
        SV lead, t;
        lead.pan = 0.5f;
        lead.pitchOffset = 3.f;
        lead.playMode = Zone::PlayMode::ONE_SHOT;
        const auto before = t;

        Zone::applyVariantEdit(t, lead, VAR_FIELD(pan));
        REQUIRE(t.pan == Approx(0.5f));
        // its neighbours were not dragged along
        REQUIRE(t.pitchOffset == Approx(before.pitchOffset));
        REQUIRE(t.playMode == before.playMode);

        Zone::applyVariantEdit(t, lead, VAR_FIELD(playMode));
        REQUIRE(t.playMode == Zone::PlayMode::ONE_SHOT);
        REQUIRE(t.pitchOffset == Approx(before.pitchOffset));
    }
}

TEST_CASE("Edit all applies one field without disturbing the rest", "[variants]")
{
    EditAllFixture f;

    // seven variants, each trimmed differently, as a user would have left them
    f.send(cmsg::AddSampleWithRange({testSample("Beep.wav"), 60, 48, 72, 0, 127}));
    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    for (int i = 1; i < 7; ++i)
        f.send(cmsg::AddSampleInZone({testSample("Beep.wav"), 0, 0, 0, i}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    std::array<int64_t, 7> starts{}, ends{}, loopStarts{};
    for (int i = 0; i < 7; ++i)
    {
        REQUIRE(zone->variantData.variants[i].active);
        auto &v = zone->variantData.variants[i];
        v.startSample = 100 * (i + 1);
        v.endSample = 5000 - 200 * i;
        v.startLoop = 50 * (i + 1);
        starts[i] = v.startSample;
        ends[i] = v.endSample;
        loopStarts[i] = v.startLoop;
    }

    zone->variantData.variants[0].pan = 0.6f;
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 0, VAR_FIELD(pan))));

    for (int i = 0; i < 7; ++i)
    {
        INFO("variant " << i);
        REQUIRE(zone->variantData.variants[i].pan == Approx(0.6f));
        // a pan edit must not drag the lead's trim across
        REQUIRE(zone->variantData.variants[i].startSample == starts[i]);
        REQUIRE(zone->variantData.variants[i].endSample == ends[i]);
        REQUIRE(zone->variantData.variants[i].startLoop == loopStarts[i]);
    }
}

TEST_CASE("Edit all leaves sample and loop positions on the lead", "[variants]")
{
    EditAllFixture f;

    f.send(cmsg::AddSampleWithRange({testSample("Beep.wav"), 60, 48, 72, 0, 127}));
    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    f.send(cmsg::AddSampleInZone({testSample("PulseSaw.wav"), 0, 0, 0, 1}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto &follower = zone->variantData.variants[0];
    follower.startSample = 100;
    follower.endSample = 8000;
    follower.startLoop = 200;
    follower.endLoop = 7000;
    follower.loopFade = 64;
    const auto before = follower;

    // drag every position on the lead, with edit-all on
    auto &lead = zone->variantData.variants[1];
    lead.startSample = 9000;
    lead.endSample = 90000;
    lead.startLoop = 9500;
    lead.endLoop = 80000;
    lead.loopFade = 4096;

    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(startSample))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(endSample))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(startLoop))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(endLoop))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(loopFade))));

    REQUIRE(follower.startSample == before.startSample);
    REQUIRE(follower.endSample == before.endSample);
    REQUIRE(follower.startLoop == before.startLoop);
    REQUIRE(follower.endLoop == before.endLoop);
    REQUIRE(follower.loopFade == before.loopFade);

    // and the lead kept what it was given
    REQUIRE(zone->variantData.variants[1].startSample == 9000);
    REQUIRE(zone->variantData.variants[1].endLoop == 80000);
}

TEST_CASE("Edit all applies across variants and preserves identity", "[variants]")
{
    EditAllFixture f;

    f.send(cmsg::AddSampleWithRange({testSample("Beep.wav"), 60, 48, 72, 0, 127}));
    auto &part = f.engine().getPatch()->getPart(0);
    REQUIRE(part->getGroups().size() == 1);
    auto &zone = part->getGroup(0)->getZone(0);

    f.send(cmsg::AddSampleInZone({testSample("PulseSaw.wav"), 0, 0, 0, 1}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    REQUIRE(zone->variantData.variants[0].active);
    REQUIRE(zone->variantData.variants[1].active);

    // give variant 0 a normalization so we can prove edit-all leaves it alone
    f.send(cmsg::NormalizeVariantAmplitude({0, true}));
    auto keptNorm = zone->variantData.variants[0].normalizationAmplitude;
    REQUIRE(keptNorm != 1.f);
    auto keptId = zone->variantData.variants[0].sampleID;

    auto &lead = zone->variantData.variants[1];
    lead.loopActive = true;
    lead.playReverse = true;
    lead.loopMode = Zone::LoopMode::LOOP_WHILE_GATED;
    lead.loopDirection = Zone::LoopDirection::ALTERNATE_DIRECTIONS;
    lead.interpolationType = scxt::dsp::InterpolationTypes::Linear;
    lead.loopCountWhenCounted = 4;
    lead.playMode = Zone::PlayMode::ON_RELEASE;
    lead.pan = 0.25f;
    lead.pitchOffset = -7.f;
    lead.amplitude = 0.5f;

    // each control is its own edit, so each is its own send
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(loopActive))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(playReverse))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(loopMode))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(loopDirection))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(interpolationType))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(loopCountWhenCounted))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(playMode))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(pan))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(pitchOffset))));
    f.send(cmsg::UpdateLeadZoneAllVariants(fanOut(*zone, 1, VAR_FIELD(amplitude))));

    const auto &shrt = zone->variantData.variants[0];
    REQUIRE(shrt.loopActive);
    REQUIRE(shrt.playReverse);
    REQUIRE(shrt.loopMode == Zone::LoopMode::LOOP_WHILE_GATED);
    REQUIRE(shrt.loopDirection == Zone::LoopDirection::ALTERNATE_DIRECTIONS);
    REQUIRE(shrt.interpolationType == scxt::dsp::InterpolationTypes::Linear);
    REQUIRE(shrt.loopCountWhenCounted == 4);
    REQUIRE(shrt.playMode == Zone::PlayMode::ON_RELEASE);
    REQUIRE(shrt.pan == Approx(0.25f));
    REQUIRE(shrt.pitchOffset == Approx(-7.f));
    REQUIRE(shrt.amplitude == Approx(0.5f));

    // identity and normalization stay with the variant
    REQUIRE(shrt.sampleID == keptId);
    REQUIRE(shrt.normalizationAmplitude == Approx(keptNorm));
}

TEST_CASE("Edit all does not write inactive variants", "[variants]")
{
    EditAllFixture f;

    f.send(cmsg::AddSampleWithRange({testSample("Beep.wav"), 60, 48, 72, 0, 127}));
    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto payload = zone->variantData.variants;
    for (auto i = 1U; i < maxVariantsPerZone; ++i)
    {
        // an active-looking payload for a slot the engine knows is empty
        payload[i].active = true;
        payload[i].pan = 0.75f;
    }

    f.send(cmsg::UpdateLeadZoneAllVariants(payload));

    for (auto i = 1U; i < maxVariantsPerZone; ++i)
    {
        REQUIRE(!zone->variantData.variants[i].active);
        REQUIRE(zone->variantData.variants[i].pan == Approx(0.f));
    }
}

TEST_CASE("A variant drag gesture is a single undo entry", "[variants][undo]")
{
    EditAllFixture f;

    f.send(cmsg::AddSampleWithRange({testSample("Beep.wav"), 60, 48, 72, 0, 127}));
    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto originalEnd = zone->variantData.variants[0].endSample;
    REQUIRE(originalEnd > 400);
    auto depthBefore = f.engine().undoManager.undoStackSize();

    // what a hot-zone drag sends: begin, an update per mouse move, end
    f.send(cmsg::BeginEdit({(int32_t)cmsg::EditSubtree::zone_variants, true, (int32_t)-1}));
    for (int i = 0; i < 12; ++i)
    {
        auto v = zone->variantData.variants[0];
        v.endSample = originalEnd - 10 * (i + 1);
        f.send(cmsg::UpdateLeadZoneSingleVariant({0, v}));
    }
    f.send(cmsg::EndEdit(true));

    REQUIRE(zone->variantData.variants[0].endSample == originalEnd - 120);
    // twelve moves, one entry
    REQUIRE(f.engine().undoManager.undoStackSize() == depthBefore + 1);

    f.sendUndo();
    REQUIRE(zone->variantData.variants[0].endSample == originalEnd);
}

TEST_CASE("A variant gesture closes so the next edit is its own entry", "[variants][undo]")
{
    EditAllFixture f;

    f.send(cmsg::AddSampleWithRange({testSample("Beep.wav"), 60, 48, 72, 0, 127}));
    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto originalEnd = zone->variantData.variants[0].endSample;
    auto depthBefore = f.engine().undoManager.undoStackSize();

    f.send(cmsg::BeginEdit({(int32_t)cmsg::EditSubtree::zone_variants, true, (int32_t)-1}));
    auto v = zone->variantData.variants[0];
    v.endSample = originalEnd - 100;
    f.send(cmsg::UpdateLeadZoneSingleVariant({0, v}));
    f.send(cmsg::EndEdit(true));

    // a later edit outside any gesture must not fold into the drag's entry
    v.pan = 0.3f;
    f.send(cmsg::UpdateLeadZoneSingleVariant({0, v}));
    REQUIRE(f.engine().undoManager.undoStackSize() == depthBefore + 2);

    f.sendUndo();
    REQUIRE(zone->variantData.variants[0].pan == Approx(0.f));
    REQUIRE(zone->variantData.variants[0].endSample == originalEnd - 100);

    f.sendUndo();
    REQUIRE(zone->variantData.variants[0].endSample == originalEnd);
}

TEST_CASE("Edit all is a single undo entry", "[variants][undo]")
{
    EditAllFixture f;

    f.send(cmsg::AddSampleWithRange({testSample("Beep.wav"), 60, 48, 72, 0, 127}));
    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    f.send(cmsg::AddSampleInZone({testSample("PulseSaw.wav"), 0, 0, 0, 1}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto before0 = zone->variantData.variants[0].pan;
    auto before1 = zone->variantData.variants[1].pan;

    auto payload = zone->variantData.variants;
    payload[0].pan = 0.4f;
    payload[1].pan = 0.4f;

    // don't drain the stack: undoing past the sample adds destroys the zone this refers to
    auto depthBefore = f.engine().undoManager.undoStackSize();

    f.send(cmsg::UpdateLeadZoneAllVariants(payload));
    REQUIRE(zone->variantData.variants[0].pan == Approx(0.4f));
    REQUIRE(zone->variantData.variants[1].pan == Approx(0.4f));

    // the whole fan-out is one entry, however many variants it touched
    REQUIRE(f.engine().undoManager.undoStackSize() == depthBefore + 1);

    f.send(cmsg::Undo(true));
    REQUIRE(zone->variantData.variants[0].pan == Approx(before0));
    REQUIRE(zone->variantData.variants[1].pan == Approx(before1));
    REQUIRE(f.engine().undoManager.undoStackSize() == depthBefore);

    f.send(cmsg::Redo(true));
    REQUIRE(zone->variantData.variants[0].pan == Approx(0.4f));
    REQUIRE(zone->variantData.variants[1].pan == Approx(0.4f));
}
