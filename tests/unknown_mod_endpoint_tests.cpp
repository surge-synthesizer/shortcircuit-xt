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

#include <memory>
#include <string>

#include <tao/json/from_string.hpp>
#include <tao/json/to_string.hpp>
#include <tao/json/value.hpp>

#include "configuration.h"
#include "console_harness.h"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "json/engine_traits.h"
#include "json/stream.h"
#include "messaging/messaging.h"
#include "modulation/group_matrix.h"
#include "modulation/voice_matrix.h"
#include "selection/selection_manager.h"

#include "test_utils.h"

// named rather than anonymous so the unity build cannot collide with other files' helpers
namespace unknown_mod_endpoint_test
{
namespace vm = scxt::voice::modulation;
namespace gm = scxt::modulation;
using TI = scxt::modulation::shared::TargetIdentifier;
using SI = scxt::modulation::shared::SourceIdentifier;

// what a newer build might add
constexpr TI futureTarget{'futr', 'targ', 0};
constexpr SI futureSource{'futr', 'srce', 0};
constexpr uint32_t futureCurve{'futr'};

constexpr SI zoneMacro{'zmac', 'mcro', 0};
constexpr SI groupMacro{'gmac', 'mcro', 0};
constexpr TI groupPan{'gout', 'pan ', 0};

template <typename Routing, typename S, typename T>
Routing routing(const S &source, const T &target)
{
    Routing r;
    r.source = source;
    r.target = target;
    r.depth = 0.5f;
    return r;
}

// one good row, then a row with each kind of unknown endpoint
template <typename RT> void fillWithUnknowns(RT &rt, const SI &source, const TI &target)
{
    using R = typename RT::Routing;
    rt.routes[0] = routing<R>(source, target);
    rt.routes[1] = routing<R>(source, futureTarget);
    rt.routes[2] = routing<R>(futureSource, target);
    rt.routes[3] = routing<R>(source, target);
    rt.routes[3].sourceVia = futureSource;
    rt.routes[4] = routing<R>(source, target);
    rt.routes[4].curve = futureCurve;
}

template <typename RT> void requireUnknownsCleared(const RT &rt, const SI &source, const TI &target)
{
    REQUIRE(rt.routes[0].source == source);
    REQUIRE(rt.routes[0].target == target);
    REQUIRE(rt.routes[0].depth == 0.5f);
    for (int i = 1; i < 5; ++i)
    {
        INFO("row " << i);
        REQUIRE(rt.routes[i].hasDefaultValues());
    }
}

struct Fixture
{
    std::unique_ptr<scxt::engine::Engine> eng;

    Fixture()
    {
        eng.reset(makeEngine());
        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        addBlankZoneToGroup(part, 0, 0, 127);
        part.macros[0].value = 1.f;
    }

    scxt::engine::Group &group() { return *eng->getPatch()->getPart(0)->getGroup(0); }
    scxt::engine::Zone &zone() { return *group().getZone(0); }

    void playNote()
    {
        eng->processNoteOnEvent(0, 0, 60, -1, 1.f, 0.f);
        for (int i = 0; i < 4; ++i)
            eng->processAudio();
        eng->processNoteOffEvent(0, 0, 60, -1, 0.f);
        for (int i = 0; i < 4; ++i)
            eng->processAudio();
    }

    void describe(bool forZone)
    {
        auto bg = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        eng->getSelectionManager()->configureAndSendZoneOrGroupModMatrixMetadata(0, 0,
                                                                                 forZone ? 0 : -1);
    }
};

TEST_CASE("Zone mod rows with unknown endpoints are cleared on unstream", "[modulation][streaming]")
{
    Fixture f;
    auto &z = f.zone();
    fillWithUnknowns(z.routingTable, zoneMacro, vm::MatrixEndpoints::MappingTarget::panA);

    REQUIRE_NOTHROW(z.setupOnUnstream(*f.eng));
    requireUnknownsCleared(z.routingTable, zoneMacro, vm::MatrixEndpoints::MappingTarget::panA);
    REQUIRE_NOTHROW(f.playNote());
    REQUIRE_NOTHROW(f.describe(true));
}

TEST_CASE("Group mod rows with unknown endpoints are cleared on unstream",
          "[modulation][streaming]")
{
    Fixture f;
    auto &g = f.group();
    fillWithUnknowns(g.routingTable, groupMacro, groupPan);

    REQUIRE_NOTHROW(g.setupOnUnstream(*f.eng));
    requireUnknownsCleared(g.routingTable, groupMacro, groupPan);
    REQUIRE_NOTHROW(f.playNote());
    REQUIRE_NOTHROW(f.describe(false));
}

// otherwise clearing unknown endpoints would drop routes that play today
TEST_CASE("Every mod target a matrix binds is registered", "[modulation]")
{
    Fixture f;

    vm::Matrix zm;
    zm.forUIMode = true;
    vm::MatrixEndpoints{nullptr}.bindTargetBaseValues(zm, f.zone());
    REQUIRE(!zm.activeTargetsToPMD.empty());
    for (const auto &[t, _] : zm.activeTargetsToPMD)
    {
        INFO(scxt::modulation::shared::identifierToString(t, "Target"));
        REQUIRE(f.eng->voiceModTargets.count(t));
    }

    gm::GroupMatrix gmat;
    gmat.forUIMode = true;
    gm::GroupMatrixEndpoints{nullptr}.bindTargetBaseValues(gmat, f.group());
    REQUIRE(!gmat.activeTargetsToPMD.empty());
    for (const auto &[t, _] : gmat.activeTargetsToPMD)
    {
        INFO(scxt::modulation::shared::identifierToString(t, "Target"));
        REQUIRE(f.eng->groupModTargets.count(t));
    }
}

TEST_CASE("Describing a mod row with an unknown target does not throw", "[modulation]")
{
    Fixture f;
    f.zone().routingTable.routes[0] =
        routing<vm::Matrix::RoutingTable::Routing>(zoneMacro, futureTarget);
    f.group().routingTable.routes[0] =
        routing<gm::GroupMatrix::RoutingTable::Routing>(groupMacro, futureTarget);

    REQUIRE_NOTHROW(f.describe(true));
    REQUIRE_NOTHROW(f.describe(false));
}

TEST_CASE("Engine state with unknown mod endpoints loads and plays", "[modulation][streaming]")
{
    Fixture f;
    fillWithUnknowns(f.zone().routingTable, zoneMacro, vm::MatrixEndpoints::MappingTarget::panA);
    fillWithUnknowns(f.group().routingTable, groupMacro, groupPan);

    std::string state;
    {
        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_DAW);
        state = scxt::json::streamEngineState(*f.eng);
    }

    Fixture reloaded;
    {
        auto bg = reloaded.eng->getMessageController()->threadingChecker.bypassChecksInScope();
        REQUIRE_NOTHROW(scxt::json::unstreamEngineState(*reloaded.eng, state));
    }

    requireUnknownsCleared(reloaded.zone().routingTable, zoneMacro,
                           vm::MatrixEndpoints::MappingTarget::panA);
    requireUnknownsCleared(reloaded.group().routingTable, groupMacro, groupPan);
    REQUIRE_NOTHROW(reloaded.playNote());
    REQUIRE_NOTHROW(reloaded.describe(true));
    REQUIRE_NOTHROW(reloaded.describe(false));
}

std::string withStreamingVersion(const std::string &json, uint64_t version)
{
    auto v = tao::json::from_string(json);
    v["streamingVersion"] = version;
    return tao::json::to_string(v);
}

bool warnedAboutNewerVersion(scxt::clients::console_ui::ConsoleHarness &th)
{
    th.stepUI(30);
    for (const auto &w : th.editor->readWarnings())
        if (std::get<1>(w) == "Saved by a newer Shortcircuit XT")
            return true;
    return false;
}

TEST_CASE("Loading engine state from a newer version warns", "[streaming]")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    std::string state;
    {
        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_DAW);
        state = scxt::json::streamEngineState(*th.engine);
    }

    SECTION("current version")
    {
        th.sendToSerialization(scxt::messaging::client::UnstreamEngineState(state));
        REQUIRE_FALSE(warnedAboutNewerVersion(th));
    }

    SECTION("newer version")
    {
        th.sendToSerialization(scxt::messaging::client::UnstreamEngineState(
            withStreamingVersion(state, scxt::currentStreamingVersion + 1)));
        REQUIRE(warnedAboutNewerVersion(th));
    }
}

TEST_CASE("Loading a part from a newer version warns", "[streaming]")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    // the part is unstreamed on this thread, so keep the audio thread out of the way
    th.audioThreadProvider.reset();
    th.stepUI();

    auto &eng = *th.engine;
    auto bg = eng.getMessageController()->threadingChecker.bypassChecksInScope();

    std::string part;
    {
        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_PART);
        part = tao::json::to_string(scxt::json::scxt_value(*eng.getPatch()->getPart(0)));
    }

    SECTION("current version")
    {
        scxt::json::unstreamPartState(eng, 0, part);
        REQUIRE_FALSE(warnedAboutNewerVersion(th));
    }

    SECTION("newer version")
    {
        scxt::json::unstreamPartState(
            eng, 0, withStreamingVersion(part, scxt::currentStreamingVersion + 1));
        REQUIRE(warnedAboutNewerVersion(th));
    }
}
} // namespace unknown_mod_endpoint_test
