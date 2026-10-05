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
 * Mod sources report whether they are unipolar, bipolar or negative only, and a row combines
 * that through its via and curve so the depth tooltip can show both directions. GH #1210.
 */

#include "catch2/catch2.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>

#include "configuration.h"
#include "dsp/processor/processor.h"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "json/engine_traits.h"
#include "json/modulation_traits.h"
#include "modulation/group_matrix.h"
#include "modulation/voice_matrix.h"
#include "voice/voice.h"

#include "test_utils.h"

namespace
{
namespace shmo = scxt::modulation::shared;
namespace vm = scxt::voice::modulation;
using SP = scxt::modulation::SourcePolarity;
using MC = scxt::modulation::ModulationCurves;
using RS = scxt::modulation::modulators::RandomStorage;
using MS = scxt::modulation::ModulatorStorage;
using VS = vm::MatrixEndpoints::Sources;
using GS = scxt::modulation::GroupMatrixEndpoints::Sources;

std::optional<SP> noVia() { return std::nullopt; }
std::optional<MC::CurveIdentifier> noCurve() { return std::nullopt; }

scxt::engine::Zone &polarityZone(scxt::engine::Engine &eng)
{
    auto &part = *eng.getPatch()->getPart(0);
    part.addGroup();
    addBlankZoneToGroup(part, 0, 0, 127);
    return *part.getGroup(0)->getZone(0);
}

struct SourceLess
{
    bool operator()(const shmo::SourceIdentifier &a, const shmo::SourceIdentifier &b) const
    {
        return std::tie(a.gid, a.tid, a.index) < std::tie(b.gid, b.tid, b.index);
    }
};

std::pair<float, float> nominalRange(SP p)
{
    if (p == SP::BIPOLAR)
        return {-1.f, 1.f};
    if (p == SP::NEGATIVE)
        return {-1.f, 0.f};
    return {0.f, 1.f};
}

SP classify(float lo, float hi)
{
    static constexpr float eps{1e-6f};
    if (lo < -eps && hi > eps)
        return SP::BIPOLAR;
    if (lo < -eps)
        return SP::NEGATIVE;
    return SP::UNIPOLAR;
}

// what curve(source * via) actually does over the nominal ranges
SP probedPolarity(const std::function<float(float)> &curve, SP source, SP via)
{
    auto [slo, shi] = nominalRange(source);
    auto [vlo, vhi] = nominalRange(via);
    auto lo = std::numeric_limits<float>::max();
    auto hi = std::numeric_limits<float>::lowest();
    static constexpr int samples{256};
    for (int i = 0; i <= samples; ++i)
        for (int j = 0; j <= samples; j += samples / 4)
        {
            auto x = (slo + (shi - slo) * i / samples) * (vlo + (vhi - vlo) * j / samples);
            auto y = curve(x);
            lo = std::min(lo, y);
            hi = std::max(hi, y);
        }
    return classify(lo, hi);
}
} // namespace

TEST_CASE("Route polarity combines source and via", "[modulation][polarity]")
{
    REQUIRE(shmo::routePolarity(SP::UNIPOLAR, noVia(), noCurve(), false) == SP::UNIPOLAR);
    REQUIRE(shmo::routePolarity(SP::BIPOLAR, noVia(), noCurve(), false) == SP::BIPOLAR);
    REQUIRE(shmo::routePolarity(SP::NEGATIVE, noVia(), noCurve(), false) == SP::NEGATIVE);

    REQUIRE(shmo::routePolarity(SP::UNIPOLAR, SP::UNIPOLAR, noCurve(), false) == SP::UNIPOLAR);
    REQUIRE(shmo::routePolarity(SP::UNIPOLAR, SP::BIPOLAR, noCurve(), false) == SP::BIPOLAR);
    REQUIRE(shmo::routePolarity(SP::BIPOLAR, SP::UNIPOLAR, noCurve(), false) == SP::BIPOLAR);
    REQUIRE(shmo::routePolarity(SP::BIPOLAR, SP::BIPOLAR, noCurve(), false) == SP::BIPOLAR);
    REQUIRE(shmo::routePolarity(SP::NEGATIVE, SP::UNIPOLAR, noCurve(), false) == SP::NEGATIVE);
    REQUIRE(shmo::routePolarity(SP::UNIPOLAR, SP::NEGATIVE, noCurve(), false) == SP::NEGATIVE);
    REQUIRE(shmo::routePolarity(SP::NEGATIVE, SP::NEGATIVE, noCurve(), false) == SP::UNIPOLAR);
    REQUIRE(shmo::routePolarity(SP::NEGATIVE, SP::BIPOLAR, noCurve(), false) == SP::BIPOLAR);
}

TEST_CASE("Multiplicative routes are unipolar", "[modulation][polarity]")
{
    for (auto p : {SP::UNIPOLAR, SP::BIPOLAR, SP::NEGATIVE})
        REQUIRE(shmo::routePolarity(p, SP::BIPOLAR, noCurve(), true) == SP::UNIPOLAR);
}

TEST_CASE("Every curve declares the polarity it produces", "[modulation][polarity]")
{
    MC::initializeCurves();
    REQUIRE(!MC::allCurves.empty());

    for (auto c : MC::allCurves)
    {
        REQUIRE(MC::curvePolarities.count(c) == 1);
        const auto &fn = MC::curveImpls.at(c);
        for (auto src : {SP::UNIPOLAR, SP::BIPOLAR, SP::NEGATIVE})
            for (auto via : {SP::UNIPOLAR, SP::BIPOLAR, SP::NEGATIVE})
            {
                INFO("curve '" << shmo::u2s(c) << "' " << MC::curveNames.at(c).second << " source "
                               << (int)src << " via " << (int)via);
                REQUIRE(shmo::routePolarity(src, via, c, false) == probedPolarity(fn, src, via));
            }
    }
}

TEST_CASE("A curve with no declaration preserves polarity", "[modulation][polarity]")
{
    auto preserves = scxt::modulation::CurvePolarity{};
    for (auto p : {SP::UNIPOLAR, SP::BIPOLAR, SP::NEGATIVE})
        REQUIRE(preserves.apply(p) == p);

    MC::initializeCurves();
    auto unknown = MC::CurveIdentifier{0};
    REQUIRE(MC::curvePolarities.count(unknown) == 0);
    for (auto p : {SP::UNIPOLAR, SP::BIPOLAR, SP::NEGATIVE})
        REQUIRE(shmo::routePolarity(p, noVia(), unknown, false) == p);
}

TEST_CASE("Zone sources report their polarity", "[modulation][polarity]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &z = polarityZone(*eng);
    auto pol = [&z](const auto &s) { return vm::sourcePolarity(z, s); };

    SECTION("Fixed sources")
    {
        REQUIRE(pol(VS::MIDISources::pbpm1SId) == SP::BIPOLAR);
        REQUIRE(pol(VS::MIDISources::modWheelSId) == SP::UNIPOLAR);
        REQUIRE(pol(VS::MIDISources::velocitySId) == SP::UNIPOLAR);
        REQUIRE(pol(VS::KeyAndPitchSources::keyTrackSId) == SP::BIPOLAR);
        REQUIRE(pol(VS::KeyAndPitchSources::pitchTrackSId) == SP::BIPOLAR);
        REQUIRE(pol(VS::KeyAndPitchSources::keySId) == SP::UNIPOLAR);
        REQUIRE(pol(VS::KeyAndPitchSources::pitchSId) == SP::UNIPOLAR);
        REQUIRE(pol(VS::MPESources::mpeBendSId) == SP::BIPOLAR);
        REQUIRE(pol(VS::MPESources::mpeTimbreSId) == SP::UNIPOLAR);
        REQUIRE(pol(VS::NoteExpressionSources::tuningSId) == SP::BIPOLAR);
        REQUIRE(pol(VS::NoteExpressionSources::panSId) == SP::UNIPOLAR);
        REQUIRE(pol(VS::VoiceSources::alternateSId) == SP::UNIPOLAR);
        REQUIRE(pol(VS::VoiceSources::alternateBipolarSId) == SP::BIPOLAR);
        REQUIRE(pol(VS::VoiceSources::alternateRotationSId) == SP::BIPOLAR);
        REQUIRE(pol(decltype(VS::midiCCSources)::ccSId(7)) == SP::UNIPOLAR);
        REQUIRE(pol(VS::egSource(0)) == SP::UNIPOLAR);
    }

    SECTION("LFOs follow their shape")
    {
        for (auto isGroupLfo : {false, true})
        {
            auto &ms = isGroupLfo ? z.parentGroup->modulatorStorage[1] : z.modulatorStorage[1];
            auto src = isGroupLfo ? VS::glfoSources_t::lfoSId(1) : VS::lfoSource(1);
            ms.modulatorShape = MS::LFO_SINE;
            ms.curveLfoStorage.unipolar = false;
            REQUIRE(pol(src) == SP::BIPOLAR);
            ms.curveLfoStorage.unipolar = true;
            REQUIRE(pol(src) == SP::UNIPOLAR);
            ms.modulatorShape = MS::STEP;
            REQUIRE(pol(src) == SP::BIPOLAR);
            ms.modulatorShape = MS::LFO_ENV;
            REQUIRE(pol(src) == SP::UNIPOLAR);
        }
    }

    SECTION("Randoms follow their style")
    {
        auto check = [&](RS::Style s, SP p) {
            z.miscSourceStorage.randoms[2].style = s;
            REQUIRE(pol(VS::rngSources_t::randomSId(2)) == p);
        };
        check(RS::UNIFORM_01, SP::UNIPOLAR);
        check(RS::UNIFORM_BIPOLAR, SP::BIPOLAR);
        check(RS::NORMAL, SP::BIPOLAR);
        check(RS::HALF_NORMAL, SP::UNIPOLAR);
        check(RS::BOOL_POS, SP::UNIPOLAR);
        check(RS::BOOL_NEG, SP::NEGATIVE);
        check(RS::TERNARY, SP::BIPOLAR);
    }

    SECTION("Macros follow their mode")
    {
        auto &m = eng->getPatch()->getPart(0)->macros[3];
        auto src = VS::MacroSources::macroSId(3);
        m.mode = scxt::engine::Macro::UNIPOLAR;
        REQUIRE(pol(src) == SP::UNIPOLAR);
        m.mode = scxt::engine::Macro::BIPOLAR;
        REQUIRE(pol(src) == SP::BIPOLAR);
        m.mode = scxt::engine::Macro::TOGGLE;
        REQUIRE(pol(src) == SP::UNIPOLAR);
    }
}

TEST_CASE("Group sources report their polarity", "[modulation][polarity]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &g = *polarityZone(*eng).parentGroup;
    auto pol = [&g](const auto &s) { return scxt::modulation::sourcePolarity(g, s); };

    REQUIRE(pol(GS::MIDISources::pbpm1SId) == SP::BIPOLAR);
    REQUIRE(pol(GS::MIDISources::modWheelSId) == SP::UNIPOLAR);
    REQUIRE(pol(GS::KeyAndPitchSources::lowKeySId) == SP::BIPOLAR);
    REQUIRE(pol(GS::KeyAndPitchSources::lastPitchSId) == SP::BIPOLAR);
    REQUIRE(pol(GS::KeyAndPitchSources::voiceCountSId) == SP::UNIPOLAR);
    REQUIRE(pol(GS::SubordinateVoiceSources::voiceCountSId) == SP::UNIPOLAR);

    g.modulatorStorage[0].modulatorShape = MS::LFO_TRI;
    REQUIRE(pol(GS::lfoSources_t::lfoSId(0)) == SP::BIPOLAR);
    g.modulatorStorage[0].curveLfoStorage.unipolar = true;
    REQUIRE(pol(GS::lfoSources_t::lfoSId(0)) == SP::UNIPOLAR);

    g.miscSourceStorage.randoms[0].style = RS::BOOL_NEG;
    REQUIRE(pol(GS::rngSources_t::randomSId(0)) == SP::NEGATIVE);

    eng->getPatch()->getPart(0)->macros[0].mode = scxt::engine::Macro::BIPOLAR;
    REQUIRE(pol(GS::MacroSources::macroSId(0)) == SP::BIPOLAR);
}

TEST_CASE("Matrix metadata carries the non-unipolar sources", "[modulation][polarity]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &z = polarityZone(*eng);
    z.modulatorStorage[0].modulatorShape = MS::LFO_SINE;
    z.miscSourceStorage.randoms[0].style = RS::BOOL_NEG;

    auto check = [](const auto &md, const auto &polFn) {
        const auto &srcs = std::get<1>(md);
        const auto &pv = std::get<4>(md);
        REQUIRE(!pv.empty());
        int listed{0};
        for (const auto &[s, nm] : srcs)
        {
            auto expected = polFn(s);
            INFO(shmo::identifierToString(s, "src"));
            REQUIRE(shmo::polarityOf(pv, s) == expected);
            if (expected != SP::UNIPOLAR)
                listed++;
        }
        REQUIRE(listed == (int)pv.size());
    };

    SECTION("Zone")
    {
        auto md = vm::getVoiceMatrixMetadata(z);
        check(md, [&](const auto &s) { return vm::sourcePolarity(z, s); });

        const auto &pv = std::get<4>(md);
        REQUIRE(shmo::polarityOf(pv, VS::lfoSource(0)) == SP::BIPOLAR);
        REQUIRE(shmo::polarityOf(pv, VS::rngSources_t::randomSId(0)) == SP::NEGATIVE);
        REQUIRE(shmo::polarityOf(pv, VS::MIDISources::velocitySId) == SP::UNIPOLAR);
    }

    SECTION("Group")
    {
        auto &g = *z.parentGroup;
        auto md = scxt::modulation::getGroupMatrixMetadata(g);
        check(md, [&](const auto &s) { return scxt::modulation::sourcePolarity(g, s); });
    }
}

TEST_CASE("A routing row reads its polarity from the metadata", "[modulation][polarity]")
{
    MC::initializeCurves();
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &z = polarityZone(*eng);
    auto pv = std::get<4>(vm::getVoiceMatrixMetadata(z));

    vm::Matrix::RoutingTable::Routing r;
    REQUIRE(shmo::routePolarity(pv, r, false) == SP::UNIPOLAR);

    r.source = VS::MIDISources::velocitySId;
    REQUIRE(shmo::routePolarity(pv, r, false) == SP::UNIPOLAR);

    r.sourceVia = VS::MIDISources::pbpm1SId;
    REQUIRE(shmo::routePolarity(pv, r, false) == SP::BIPOLAR);
    REQUIRE(shmo::routePolarity(pv, r, true) == SP::UNIPOLAR);

    // any curve declared to fold bipolar into unipolar
    auto folds = std::find_if(MC::allCurves.begin(), MC::allCurves.end(), [](auto c) {
        return MC::curvePolarity(c, SP::BIPOLAR) == SP::UNIPOLAR;
    });
    REQUIRE(folds != MC::allCurves.end());
    r.curve = *folds;
    REQUIRE(shmo::routePolarity(pv, r, false) == SP::UNIPOLAR);
}

TEST_CASE("Source polarities survive the trip to the client", "[modulation][polarity]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &z = polarityZone(*eng);
    z.miscSourceStorage.randoms[1].style = RS::BOOL_NEG;
    auto md = vm::getVoiceMatrixMetadata(z);

    auto s = tao::json::to_string(scxt::json::scxt_value(md));
    tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
        consumer;
    tao::json::events::from_string(consumer, s);
    vm::voiceMatrixMetadata_t back;
    consumer.value.to(back);

    REQUIRE(std::get<4>(back) == std::get<4>(md));
    REQUIRE(shmo::polarityOf(std::get<4>(back), VS::rngSources_t::randomSId(1)) == SP::NEGATIVE);
}

TEST_CASE("No source marked unipolar goes negative while playing", "[modulation][polarity]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &z = polarityZone(*eng);
    auto &part = *eng->getPatch()->getPart(0);

    // an empty zone's voice ends on its first block, so give it something to sustain
    {
        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        z.setProcessorType(0, scxt::dsp::processor::proct_osc_sineplus);
    }

    // a routed lfo runs, so the sweep sees a bipolar one move
    auto &row = z.routingTable.routes[0];
    row.active = true;
    vm::MatrixEndpoints ep{nullptr};
    row.source = VS::lfoSource(0);
    row.target = ep.outputTarget.panT;
    row.depth = 0.f;
    z.modulatorStorage[0].modulatorShape = MS::LFO_SINE;
    z.modulatorStorage[0].rate = 4.f;
    z.onRoutingChanged();

    for (auto &r : z.miscSourceStorage.randoms)
        r.style = RS::TERNARY;
    for (auto &m : part.macros)
    {
        m.mode = scxt::engine::Macro::BIPOLAR;
        m.value = -1.f;
    }

    std::map<shmo::SourceIdentifier, std::pair<float, float>, SourceLess> seen;

    for (int key = 24; key <= 108; key += 7)
    {
        part.pitchBendValue = (key % 2) ? -1.f : 1.f;
        eng->processNoteOnEvent(0, 0, key, -1, 0.8f, 0.f);
        for (int b = 0; b < 64; ++b)
        {
            eng->processAudio();
            for (int i = 0; i < (int)scxt::maxVoices; ++i)
            {
                const auto *v = z.voiceWeakPointers[i];
                if (!v || !v->isVoiceAssigned || !v->modMatrix)
                    continue;
                for (const auto &[s, p] : v->modMatrix->sourceValues)
                {
                    auto [it, fresh] = seen.try_emplace(s, *p, *p);
                    it->second.first = std::min(it->second.first, *p);
                    it->second.second = std::max(it->second.second, *p);
                }
            }
        }
        eng->processNoteOffEvent(0, 0, key, -1, 0.f);
    }

    REQUIRE(!seen.empty());
    int negativesSeen{0};
    for (const auto &[s, mm] : seen)
    {
        // a zone with no sample plays variant -1
        if (s == VS::VoiceSources::variantCountSId)
            continue;
        auto p = vm::sourcePolarity(z, s);
        INFO(shmo::identifierToString(s, "src") << " min " << mm.first << " max " << mm.second);
        if (mm.first < -1e-5)
        {
            negativesSeen++;
            REQUIRE(p != SP::UNIPOLAR);
        }
        if (mm.second > 1e-5)
            REQUIRE(p != SP::NEGATIVE);
    }
    // the sweep has to actually reach below zero to mean anything
    REQUIRE(negativesSeen >= 3);
}
