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
#include "console_harness.h"
#include "selection/selection_manager.h"

namespace cmsg = scxt::messaging::client;
using ZoneAddress = scxt::selection::SelectionManager::ZoneAddress;

// Helper: set the depth of a routing table row for the selected zone(s)
// via UpdateZoneRoutingRow, then step the UI to process it.
static void setZoneRouteDepth(scxt::clients::console_ui::ConsoleHarness &th, int part, int group,
                              int zone, int rowIdx, float depth)
{
    auto &z = th.engine->getPatch()->getPart(part)->getGroup(group)->getZone(zone);
    auto row = z->routingTable.routes[rowIdx];
    row.depth = depth;
    th.sendToSerialization(cmsg::UpdateZoneRoutingRow({rowIdx, row, false}));
    th.stepUI();
}

TEST_CASE("Mod Matrix Row Swap")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    // Add a blank zone; it is auto-selected after creation
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.stepUI();

    // Confirm selection is at zone 0
    th.sendToSerialization(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
    th.stepUI();

    // Set distinctive depth values on rows 0 and 1
    setZoneRouteDepth(th, 0, 0, 0, 0, 0.25f);
    setZoneRouteDepth(th, 0, 0, 0, 1, 0.75f);

    auto &z = th.engine->getPatch()->getPart(0)->getGroup(0)->getZone(0);
    REQUIRE(z->routingTable.routes[0].depth == Approx(0.25f));
    REQUIRE(z->routingTable.routes[1].depth == Approx(0.75f));

    // SWAP rows 0 and 1 (forZone=true, fromRow=0, toRow=1, isMove=false)
    th.sendToSerialization(cmsg::ReorderModRow({true, 0, 1, false}));
    th.stepUI();

    REQUIRE(z->routingTable.routes[0].depth == Approx(0.75f));
    REQUIRE(z->routingTable.routes[1].depth == Approx(0.25f));
}

TEST_CASE("Mod Matrix Row Move Forward")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.stepUI();

    th.sendToSerialization(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
    th.stepUI();

    // Set depths for rows 0, 1, 2
    setZoneRouteDepth(th, 0, 0, 0, 0, 0.10f);
    setZoneRouteDepth(th, 0, 0, 0, 1, 0.20f);
    setZoneRouteDepth(th, 0, 0, 0, 2, 0.30f);

    auto &z = th.engine->getPatch()->getPart(0)->getGroup(0)->getZone(0);

    // MOVE row 0 to position 2: [0.10, 0.20, 0.30] → [0.20, 0.30, 0.10]
    // (forZone=true, fromRow=0, toRow=2, isMove=true)
    th.sendToSerialization(cmsg::ReorderModRow({true, 0, 2, true}));
    th.stepUI();

    REQUIRE(z->routingTable.routes[0].depth == Approx(0.20f));
    REQUIRE(z->routingTable.routes[1].depth == Approx(0.30f));
    REQUIRE(z->routingTable.routes[2].depth == Approx(0.10f));
}

TEST_CASE("Mod Matrix Row Move Backward")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.stepUI();

    th.sendToSerialization(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
    th.stepUI();

    setZoneRouteDepth(th, 0, 0, 0, 0, 0.10f);
    setZoneRouteDepth(th, 0, 0, 0, 1, 0.20f);
    setZoneRouteDepth(th, 0, 0, 0, 2, 0.30f);

    auto &z = th.engine->getPatch()->getPart(0)->getGroup(0)->getZone(0);

    // MOVE row 2 to position 0: [0.10, 0.20, 0.30] → [0.30, 0.10, 0.20]
    // (forZone=true, fromRow=2, toRow=0, isMove=true)
    th.sendToSerialization(cmsg::ReorderModRow({true, 2, 0, true}));
    th.stepUI();

    REQUIRE(z->routingTable.routes[0].depth == Approx(0.30f));
    REQUIRE(z->routingTable.routes[1].depth == Approx(0.10f));
    REQUIRE(z->routingTable.routes[2].depth == Approx(0.20f));
}

TEST_CASE("Mod Matrix Row Duplicate via UpdateZoneRoutingRow")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.stepUI();

    th.sendToSerialization(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
    th.stepUI();

    setZoneRouteDepth(th, 0, 0, 0, 0, 0.42f);

    auto &z = th.engine->getPatch()->getPart(0)->getGroup(0)->getZone(0);
    REQUIRE(z->routingTable.routes[0].depth == Approx(0.42f));

    // Duplicate row 0 → row 3 by reading the row and writing it to index 3
    auto row = z->routingTable.routes[0];
    th.sendToSerialization(cmsg::UpdateZoneRoutingRow({3, row, false}));
    th.stepUI();

    REQUIRE(z->routingTable.routes[0].depth == Approx(0.42f));
    REQUIRE(z->routingTable.routes[3].depth == Approx(0.42f));
}
