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

#include <tao/json/value.hpp>

#include "configuration.h"
#include "console_harness.h"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "json/engine_traits.h"
#include "messaging/client/client_messages.h"
#include "modulation/group_matrix.h"
#include "modulation/voice_matrix.h"
#include "selection/selection_manager.h"

#include "test_utils.h"

// named rather than anonymous so the unity build cannot collide with other files' helpers
namespace group_eg_test
{
namespace vm = scxt::voice::modulation;
namespace gm = scxt::modulation;
using TI = scxt::modulation::shared::TargetIdentifier;

constexpr TI groupPan{'gout', 'pan ', 0};

struct Fixture
{
    std::unique_ptr<scxt::engine::Engine> eng;

    Fixture()
    {
        eng.reset(makeEngine());
        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();
        addBlankZoneToGroup(part, 0, 0, 127);
    }

    scxt::engine::Group &group() { return *eng->getPatch()->getPart(0)->getGroup(0); }
    scxt::engine::Zone &zone() { return *group().getZone(0); }
};
} // namespace group_eg_test

using namespace group_eg_test;

TEST_CASE("Every group EG is a mod source in both matrices", "[modulation][groupeg]")
{
    Fixture f;
    const auto &gs = f.group().endpoints.sources.egSource;
    const auto &zs = vm::sourcesForScanning().gegSources;
    REQUIRE(gs.size() == scxt::egsPerGroup);
    REQUIRE(zs.size() == scxt::egsPerGroup);
    for (int i = 0; i < scxt::egsPerGroup; ++i)
    {
        INFO("group EG " << i);
        REQUIRE(f.eng->groupModSources.count(gs[i]));
        REQUIRE(f.eng->voiceModSources.count(zs[i]));
        for (int j = 0; j < i; ++j)
            REQUIRE(!(gs[i] == gs[j]));
    }
}

TEST_CASE("The first two group EG sources keep their streamed identifiers", "[modulation][groupeg]")
{
    Fixture f;
    const auto &gs = f.group().endpoints.sources.egSource;
    REQUIRE(gs[0] == gm::GroupMatrixConfig::SourceIdentifier{'greg', 'eg1 ', 0});
    REQUIRE(gs[1] == gm::GroupMatrixConfig::SourceIdentifier{'greg', 'eg2 ', 0});
}

TEST_CASE("A group mod row runs only the group EG it reads", "[modulation][groupeg]")
{
    for (int i = 0; i < scxt::egsPerGroup; ++i)
    {
        INFO("group EG " << i);
        Fixture f;
        auto &g = f.group();
        g.routingTable.routes[0].source = g.endpoints.sources.egSource[i];
        g.routingTable.routes[0].target = groupPan;
        g.routingTable.routes[0].depth = 0.5f;
        g.onRoutingChanged();

        for (int j = 0; j < scxt::egsPerGroup; ++j)
            REQUIRE(g.egsActive[j] == (i == j));
    }
}

TEST_CASE("A zone mod row runs only the group EG it reads", "[modulation][groupeg]")
{
    for (int i = 0; i < scxt::egsPerGroup; ++i)
    {
        INFO("group EG " << i);
        Fixture f;
        auto &z = f.zone();
        z.routingTable.routes[0].source = vm::sourcesForScanning().gegSources[i];
        z.routingTable.routes[0].target = vm::MatrixEndpoints::MappingTarget::panA;
        z.routingTable.routes[0].depth = 0.5f;
        z.onRoutingChanged();

        for (int j = 0; j < scxt::egsPerGroup; ++j)
        {
            REQUIRE(z.gegsActive[j] == (i == j));
            REQUIRE(f.group().egsActive[j] == (i == j));
        }
    }
}

TEST_CASE("The last group EG runs while a note is held", "[modulation][groupeg]")
{
    auto last = scxt::egsPerGroup - 1;
    Fixture f;
    auto &g = f.group();
    g.gegStorage[last].s = 0.25f;
    g.routingTable.routes[0].source = g.endpoints.sources.egSource[last];
    g.routingTable.routes[0].target = groupPan;
    g.routingTable.routes[0].depth = 0.5f;
    g.onRoutingChanged();

    f.eng->processNoteOnEvent(0, 0, 60, -1, 1.f, 0.f);
    for (int i = 0; i < 100; ++i)
        f.eng->processAudio();

    // attack lands on 1, so only a processed EG decays to its sustain
    REQUIRE(g.eg[last].outBlock0 == Approx(0.25f).margin(0.01f));
}

TEST_CASE("A group streamed with fewer group EGs unstreams", "[streaming][groupeg]")
{
    Fixture f;
    f.group().gegStorage[0].a = 0.2f;
    f.group().gegStorage[1].a = 0.3f;

    auto v = scxt::json::scxt_value(f.group());
    auto &gegs = v.at("gegStorage").get_array();
    REQUIRE(gegs.size() == scxt::egsPerGroup);
    // what a patch saved when a group had two EGs looks like
    gegs.resize(2);

    Fixture reloaded;
    for (auto &geg : reloaded.group().gegStorage)
        geg.a = 0.9f;
    REQUIRE_NOTHROW(v.to(reloaded.group()));

    const auto &res = reloaded.group().gegStorage;
    REQUIRE(res[0].a == Approx(0.2f));
    REQUIRE(res[1].a == Approx(0.3f));
    for (int i = 2; i < scxt::egsPerGroup; ++i)
    {
        INFO("group EG " << i);
        REQUIRE(res[i].a == Approx(0.9f));
    }
}

TEST_CASE("A group streamed with more group EGs unstreams", "[streaming][groupeg]")
{
    Fixture f;
    f.group().gegStorage[scxt::egsPerGroup - 1].a = 0.4f;

    auto v = scxt::json::scxt_value(f.group());
    auto &gegs = v.at("gegStorage").get_array();
    // a newer build with more group EGs than this one
    while (gegs.size() < scxt::egsPerGroup + 3)
        gegs.push_back(gegs[0]);

    Fixture reloaded;
    REQUIRE_NOTHROW(v.to(reloaded.group()));
    REQUIRE(reloaded.group().gegStorage[scxt::egsPerGroup - 1].a == Approx(0.4f));
}

TEST_CASE("The last group EG edits and undoes through messages", "[undo][groupeg]")
{
    namespace cmsg = scxt::messaging::client;
    using sac = scxt::selection::SelectionManager::SelectActionContents;
    auto last = scxt::egsPerGroup - 1;

    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    auto send = [&th](const auto &msg) {
        th.sendToSerialization(msg);
        th.stepUI(10);
    };

    send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    sac sa(0, 0, -1, true, true, true);
    sa.forZone = false;
    send(cmsg::ApplySelectActions({sa}));

    auto &grp = th.engine->getPatch()->getPart(0)->getGroup(0);
    auto orig = grp->gegStorage[last];

    send(cmsg::UpdateZoneOrGroupEGFloatValue(
        {false, last, offsetof(scxt::modulation::modulators::AdsrStorage, a), 0.7f}));
    REQUIRE(grp->gegStorage[last].a == Approx(0.7f));
    REQUIRE(grp->gegStorage[0].a == Approx(orig.a));

    auto next = grp->gegStorage[last];
    next.gateMode = scxt::modulation::modulators::AdsrStorage::GateMode::ONESHOT;
    send(cmsg::UpdateFullAdsrStorageForGroupsOrZones({false, last, next}));
    REQUIRE(grp->gegStorage[last].gateMode == next.gateMode);

    send(cmsg::Undo(true));
    REQUIRE(grp->gegStorage[last].gateMode == orig.gateMode);
    send(cmsg::Undo(true));
    REQUIRE(grp->gegStorage[last].a == Approx(orig.a));
}
