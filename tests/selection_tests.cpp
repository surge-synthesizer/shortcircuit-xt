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

TEST_CASE("Create a Single Blank Zone and it is Selected")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();

    th.stepUI();
    th.sendToSerialization(scxt::messaging::client::AddBlankZone({0, 0, 60, 72, 0, 64}));
    th.stepUI();

    REQUIRE(th.engine->getSelectionManager()->leadZone[0] ==
            scxt::selection::SelectionManager::ZoneAddress{0, 0, 0});
}

TEST_CASE("Create five zones then multi-select")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;

    using abz = cmsg::AddBlankZone;
    using zad = scxt::selection::SelectionManager::ZoneAddress;
    using asa = cmsg::ApplySelectActions;
    using sac = scxt::selection::SelectionManager::SelectActionContents;

    auto sel = [&th](auto p, auto g, auto z, auto s, auto d, auto ld) {
        th.sendToSerialization(cmsg::ApplySelectActions({{p, g, z, s, d, ld}}));
    };
    const auto &sm = th.engine->getSelectionManager();
    REQUIRE(sm->selectedPart == 0);

    th.sendToSerialization(abz({0, 0, 60, 72, 0, 64}));
    th.sendToSerialization(abz({0, 0, 60, 72, 65, 127}));
    th.sendToSerialization(abz({0, 0, 73, 82, 0, 64}));
    th.sendToSerialization(abz({0, 0, 73, 82, 65, 127}));
    th.sendToSerialization(abz({0, 0, 83, 92, 0, 127}));
    th.stepUI();

    REQUIRE(sm->leadZone[0] == zad{0, 0, 4});
    REQUIRE(sm->allSelectedZones[0].size() == 1);

    INFO("Single select other zones");
    // remember SAC is p/g/z select, distinct, as-lead
    sel(0, 0, 2, true, true, true);
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 2});
    REQUIRE(sm->allSelectedZones[0].size() == 1);

    INFO("Single select a second zone not as lead");
    sel(0, 0, 0, true, false, false);
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 2});
    REQUIRE(sm->allSelectedZones[0].size() == 2);

    INFO("Swap the lead");
    sel(0, 0, 0, true, false, true);
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 0});
    REQUIRE(sm->allSelectedZones[0].size() == 2);

    INFO("Unselect the non-lead");
    sel(0, 0, 2, false, false, false);
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 0});
    REQUIRE(sm->allSelectedZones[0].size() == 1);

    INFO("Select two more groups as non-lead and lead");
    sel(0, 0, 1, true, false, true);
    sel(0, 0, 3, true, false, false);
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 1});
    REQUIRE(sm->allSelectedZones[0].size() == 3);

    INFO("Select a different zone disticnt and get it lead with set clear");
    sel(0, 0, 4, true, true, true);
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 4});
    REQUIRE(sm->allSelectedZones[0].size() == 1);
}

TEST_CASE("Selecting an empty group makes it the lead group")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    using zad = scxt::selection::SelectionManager::ZoneAddress;
    using sac = scxt::selection::SelectionManager::SelectActionContents;

    auto selGroup = [&th](int p, int g, bool s, bool d, bool ld) {
        sac sa(p, g, -1, s, d, ld);
        sa.forZone = false;
        th.sendToSerialization(cmsg::ApplySelectActions({sa}));
    };

    const auto &sm = th.engine->getSelectionManager();

    // group 0 with one zone, plus an empty group 1
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 60, 72, 0, 127}));
    th.sendToSerialization(cmsg::CreateGroup(0));
    th.stepUI();

    REQUIRE(th.engine->getPatch()->getPart(0)->getGroups().size() == 2);
    REQUIRE(th.engine->getPatch()->getPart(0)->getGroup(1)->getZones().empty());

    INFO("Distinct-select the empty group as lead");
    selGroup(0, 1, true, true, true);
    th.stepUI();

    REQUIRE(sm->leadGroup[0] == zad{0, 1, -1});
    REQUIRE(sm->allSelectedGroups[0].size() == 1);
    REQUIRE(sm->allSelectedGroups[0].count(zad{0, 1, -1}) == 1);
}

TEST_CASE("Selecting an empty group clears prior zone selections")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    using zad = scxt::selection::SelectionManager::ZoneAddress;
    using sac = scxt::selection::SelectionManager::SelectActionContents;

    auto selGroup = [&th](int p, int g, bool s, bool d, bool ld) {
        sac sa(p, g, -1, s, d, ld);
        sa.forZone = false;
        th.sendToSerialization(cmsg::ApplySelectActions({sa}));
    };

    const auto &sm = th.engine->getSelectionManager();

    // two zones in group 0, empty group 1
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    th.sendToSerialization(cmsg::CreateGroup(0));
    th.stepUI();

    REQUIRE(sm->leadZone[0] == zad{0, 0, 1});
    REQUIRE(!sm->allSelectedZones[0].empty());

    INFO("Distinct-select the empty group; zone state should clear");
    selGroup(0, 1, true, true, true);
    th.stepUI();

    REQUIRE(sm->leadGroup[0] == zad{0, 1, -1});
    REQUIRE(sm->allSelectedZones[0].empty());
    REQUIRE(sm->leadZone[0] == zad{});
}

TEST_CASE("AddBlankZone after selecting empty group lands in that group")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    using zad = scxt::selection::SelectionManager::ZoneAddress;
    using sac = scxt::selection::SelectionManager::SelectActionContents;

    auto selGroup = [&th](int p, int g, bool s, bool d, bool ld) {
        sac sa(p, g, -1, s, d, ld);
        sa.forZone = false;
        th.sendToSerialization(cmsg::ApplySelectActions({sa}));
    };

    const auto &sm = th.engine->getSelectionManager();

    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 60, 72, 0, 127}));
    th.sendToSerialization(cmsg::CreateGroup(0));
    th.stepUI();

    selGroup(0, 1, true, true, true);
    th.stepUI();

    REQUIRE(sm->leadGroup[0] == zad{0, 1, -1});
    REQUIRE(th.engine->getPatch()->getPart(0)->getGroup(1)->getZones().empty());

    INFO("Create a blank zone explicitly targeting the empty group");
    th.sendToSerialization(cmsg::AddBlankZone({0, 1, 48, 72, 0, 127}));
    th.stepUI();

    REQUIRE(th.engine->getPatch()->getPart(0)->getGroup(0)->getZones().size() == 1);
    REQUIRE(th.engine->getPatch()->getPart(0)->getGroup(1)->getZones().size() == 1);
    REQUIRE(sm->leadZone[0] == zad{0, 1, 0});
}

TEST_CASE("Switching from empty group to non-empty group selects its zones")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    using zad = scxt::selection::SelectionManager::ZoneAddress;
    using sac = scxt::selection::SelectionManager::SelectActionContents;

    auto selGroup = [&th](int p, int g, bool s, bool d, bool ld) {
        sac sa(p, g, -1, s, d, ld);
        sa.forZone = false;
        th.sendToSerialization(cmsg::ApplySelectActions({sa}));
    };

    const auto &sm = th.engine->getSelectionManager();

    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    th.sendToSerialization(cmsg::CreateGroup(0));
    th.stepUI();

    selGroup(0, 1, true, true, true);
    th.stepUI();
    REQUIRE(sm->leadGroup[0] == zad{0, 1, -1});
    REQUIRE(sm->allSelectedZones[0].empty());

    INFO("Now distinct-select the non-empty group 0");
    selGroup(0, 0, true, true, true);
    th.stepUI();

    REQUIRE(sm->leadGroup[0] == zad{0, 0, -1});
    REQUIRE(sm->allSelectedGroups[0].size() == 1);
    REQUIRE(sm->allSelectedGroups[0].count(zad{0, 0, -1}) == 1);
    REQUIRE(sm->allSelectedZones[0].size() == 2);
    REQUIRE(sm->leadZone[0].part == 0);
    REQUIRE(sm->leadZone[0].group == 0);
}