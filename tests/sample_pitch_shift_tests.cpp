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

#include <cmath>
#include <filesystem>
#include <memory>
#include <string>

#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>

#include "configuration.h"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "json/engine_traits.h"
#include "json/modulation_traits.h"
#include "messaging/messaging.h"
#include "modulation/voice_matrix.h"
#include "voice/voice.h"

#include "test_utils.h"

// named rather than anonymous so the unity build cannot collide with other files' helpers
namespace sample_pitch_shift_test
{
namespace vm = scxt::voice::modulation;
using Routing = vm::Matrix::RoutingTable::Routing;
using ST = vm::MatrixEndpoints::SampleTarget;
using MT = vm::MatrixEndpoints::MappingTarget;

struct PitchShiftFixture
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Part *part{nullptr};
    scxt::engine::Zone *zone{nullptr};

    PitchShiftFixture()
    {
        eng.reset(makeEngine());
        part = eng->getPatch()->getPart(0).get();
        part->addGroup();

        auto z = std::make_unique<scxt::engine::Zone>();
        z->mapping.keyboardRange = {48, 84};
        z->mapping.velocityRange = {0, 127};
        z->mapping.rootKey = 60;
        z->initialize();
        part->getGroup(0)->addZone(z);
        zone = part->getGroup(0)->getZone(0).get();

        auto p = samplePath("WavStereo48k.wav");
        REQUIRE(std::filesystem::exists(p));

        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        auto sid = eng->getSampleManager()->loadSampleByPath(p);
        REQUIRE(sid.has_value());
        zone->variantData.variants[0].sampleID = *sid;
        zone->variantData.variants[0].active = true;
        REQUIRE(zone->attachToSample(*eng->getSampleManager(), 0,
                                     scxt::engine::Zone::SampleInformationRead::ENDPOINTS));

        // a constant full scale source
        part->macros[0].value = 1.f;
    }

    void route(const vm::MatrixConfig::TargetIdentifier &target, float depth)
    {
        auto &row = zone->routingTable.routes[0];
        row.active = true;
        row.source = vm::sourcesForScanning().macroSources.macros[0];
        row.target = target;
        row.depth = depth;
        zone->onRoutingChanged();
    }

    double ratioOnRootKey()
    {
        eng->processNoteOnEvent(0, 0, 60, -1, 1.f, 0.f);
        eng->processAudio();

        scxt::voice::Voice *newest{nullptr};
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned &&
                (!newest || v->voiceCreationId > newest->voiceCreationId))
                newest = v;
        }
        REQUIRE(newest);
        REQUIRE(newest->numGeneratorsActive == 1);
        auto res = (double)newest->GD[0].ratio;

        eng->processNoteOffEvent(0, 0, 60, -1, 0.f);
        for (int i = 0; i < 200; ++i)
            eng->processAudio();
        return res;
    }
};

double semitonesUp(double base, double semis) { return base * std::pow(2.0, semis / 12.0); }

Routing routingTo(const vm::MatrixConfig::TargetIdentifier &target, float depth)
{
    Routing r;
    r.active = true;
    r.source = vm::sourcesForScanning().macroSources.macros[0];
    r.target = target;
    r.depth = depth;
    return r;
}

Routing roundTrip(const Routing &in)
{
    auto s = tao::json::to_string(scxt::json::scxt_value(in));
    tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
        consumer;
    tao::json::events::from_string(consumer, s);
    auto val = std::move(consumer.value);
    Routing out;
    val.to(out);
    return out;
}

// the version before sample tune and playback ratio became pitch shift
constexpr uint64_t priorVersion{0x2026'08'11};

TEST_CASE("Pitch shift moves the sample by semitones", "[modulation]")
{
    PitchShiftFixture f;
    auto base = f.ratioOnRootKey();
    REQUIRE(base > 0);

    SECTION("An octave up")
    {
        f.route(ST::pitchShiftA, 12.f / 96.f);
        REQUIRE(f.ratioOnRootKey() == Approx(semitonesUp(base, 12)).epsilon(1e-5));
    }

    SECTION("A fifth down")
    {
        f.route(ST::pitchShiftA, -7.f / 96.f);
        REQUIRE(f.ratioOnRootKey() == Approx(semitonesUp(base, -7)).epsilon(1e-5));
    }

    SECTION("Full depth stops at four octaves")
    {
        f.route(ST::pitchShiftA, 1.f);
        REQUIRE(f.ratioOnRootKey() == Approx(semitonesUp(base, 48)).epsilon(1e-5));
    }
}

TEST_CASE("Fine pitch shift moves the sample by cents", "[modulation]")
{
    PitchShiftFixture f;
    auto base = f.ratioOnRootKey();
    REQUIRE(base > 0);

    SECTION("Fifty cents up")
    {
        f.route(ST::finePitchShiftA, 50.f / 200.f);
        REQUIRE(f.ratioOnRootKey() == Approx(semitonesUp(base, 0.5)).epsilon(1e-5));
    }

    SECTION("Full depth stops at a semitone")
    {
        f.route(ST::finePitchShiftA, -1.f);
        REQUIRE(f.ratioOnRootKey() == Approx(semitonesUp(base, -1)).epsilon(1e-5));
    }
}

TEST_CASE("Pitch shift targets replace playback ratio in the menu", "[modulation]")
{
    PitchShiftFixture f;
    auto md = vm::getVoiceMatrixMetadata(*f.zone);
    const auto &targets = std::get<2>(md);

    bool sawShift{false}, sawFine{false};
    for (const auto &t : targets)
    {
        const auto &id = std::get<0>(t);
        const auto &dn = std::get<1>(t);
        REQUIRE_FALSE(id == MT::legacyPlaybackRatioA);
        if (id == ST::pitchShiftA)
        {
            sawShift = true;
            REQUIRE(vm::displayPath(dn) == "Sample");
            REQUIRE(vm::displayName(dn) == "Pitch Shift");
        }
        if (id == ST::finePitchShiftA)
        {
            sawFine = true;
            REQUIRE(vm::displayPath(dn) == "Sample");
            REQUIRE(vm::displayName(dn) == "Fine Pitch Shift");
        }
    }
    REQUIRE(sawShift);
    REQUIRE(sawFine);
}

TEST_CASE("Prior sample pitch routes unstream onto pitch shift", "[modulation][streaming]")
{
    SECTION("A current stream round trips untouched")
    {
        scxt::engine::Engine::UnstreamGuard sg(scxt::currentStreamingVersion);
        auto r = roundTrip(routingTo(ST::pitchShiftA, 0.1f));
        REQUIRE(*r.target == ST::pitchShiftA);
        REQUIRE(r.depth == Approx(0.1f));
    }

    SECTION("An in process stream is not converted")
    {
        auto r = roundTrip(routingTo(ST::pitchShiftA, 0.1f));
        REQUIRE(r.depth == Approx(0.1f));
    }

    scxt::engine::Engine::UnstreamGuard sg(priorVersion);

    SECTION("Sample tune keeps its semitones")
    {
        auto r = roundTrip(routingTo(ST::pitchShiftA, 12.f / 192.f));
        REQUIRE(*r.target == ST::pitchShiftA);
        REQUIRE(r.depth == Approx(12.f / 96.f));
    }

    SECTION("Sample tune past four octaves clamps")
    {
        auto r = roundTrip(routingTo(ST::pitchShiftA, -0.75f));
        REQUIRE(r.depth == Approx(-1.f));
    }

    SECTION("A doubled playback ratio becomes an octave")
    {
        auto r = roundTrip(routingTo(MT::legacyPlaybackRatioA, 0.5f));
        REQUIRE(*r.target == ST::pitchShiftA);
        REQUIRE(r.depth == Approx(12.f / 96.f));
        REQUIRE(r.active);
        REQUIRE(*r.source == vm::sourcesForScanning().macroSources.macros[0]);
    }

    SECTION("Full playback ratio becomes a tripled rate")
    {
        auto r = roundTrip(routingTo(MT::legacyPlaybackRatioA, 1.f));
        REQUIRE(r.depth == Approx(12.f * std::log2(3.f) / 96.f));
    }

    SECTION("Negative playback ratio pitches down")
    {
        auto r = roundTrip(routingTo(MT::legacyPlaybackRatioA, -0.5f));
        REQUIRE(*r.target == ST::pitchShiftA);
        REQUIRE(r.depth == Approx(-12.f / 96.f));
    }

    SECTION("Other targets are left alone")
    {
        auto r = roundTrip(routingTo(MT::pitchOffsetA, 0.3f));
        REQUIRE(*r.target == MT::pitchOffsetA);
        REQUIRE(r.depth == Approx(0.3f));
    }
}

TEST_CASE("A prior playback ratio route still plays an octave up", "[modulation][streaming]")
{
    PitchShiftFixture f;
    auto base = f.ratioOnRootKey();
    REQUIRE(base > 0);

    Routing converted;
    {
        scxt::engine::Engine::UnstreamGuard sg(priorVersion);
        converted = roundTrip(routingTo(MT::legacyPlaybackRatioA, 0.5f));
    }
    f.zone->routingTable.routes[0] = converted;
    f.zone->onRoutingChanged();

    REQUIRE(f.ratioOnRootKey() == Approx(2.0 * base).epsilon(1e-5));
}
} // namespace sample_pitch_shift_test
