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
#include "engine/feature_enums.h"

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

// ---- Group fold state (sidebar tree) ----

namespace
{
// Find the group row for (part, group) in the pgz structure and return its
// features bitfield. Returns 0 if not found (so feature tests are robust to
// missing rows).
int32_t pgzGroupFeatures(scxt::clients::console_ui::ConsoleHarness &th, int part, int group)
{
    auto pgz = th.engine->getPartGroupZoneStructure();
    for (const auto &b : pgz)
        if (b.address.part == part && b.address.group == group && b.address.zone == -1)
            return b.features;
    return 0;
}

// Make `n` extra groups in part 0 (engine starts with group 0). After calling,
// part 0 has exactly n+1 groups (indices 0..n).
void makeGroups(scxt::clients::console_ui::ConsoleHarness &th, int n)
{
    namespace cmsg = scxt::messaging::client;
    for (int i = 0; i < n; ++i)
        th.sendToSerialization(cmsg::CreateGroup(0));
    th.stepUI();
}
} // namespace

TEST_CASE("Fold: setGroupCollapsed toggles a single group")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    makeGroups(th, 5); // 5 groups: 0..4

    const auto &sm = th.engine->getSelectionManager();
    REQUIRE(sm->collapsedGroupsByPart[0].empty());
    REQUIRE(!sm->isGroupCollapsed(0, 2));

    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 2, true}));
    th.stepUI();
    REQUIRE(sm->isGroupCollapsed(0, 2));
    REQUIRE(sm->collapsedGroupsByPart[0].size() == 1);

    INFO("FOLDED bit shows up on the structure broadcast for that group");
    REQUIRE((pgzGroupFeatures(th, 0, 2) & scxt::engine::GroupZoneFeatures::FOLDED) != 0);
    REQUIRE((pgzGroupFeatures(th, 0, 1) & scxt::engine::GroupZoneFeatures::FOLDED) == 0);

    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 2, false}));
    th.stepUI();
    REQUIRE(!sm->isGroupCollapsed(0, 2));
    REQUIRE(sm->collapsedGroupsByPart[0].empty());
    REQUIRE((pgzGroupFeatures(th, 0, 2) & scxt::engine::GroupZoneFeatures::FOLDED) == 0);
}

TEST_CASE("Fold: setAllGroupsCollapsed true/false")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    makeGroups(th, 5); // 5 groups
    const auto &sm = th.engine->getSelectionManager();

    th.sendToSerialization(cmsg::SetAllGroupsCollapsed({0, true}));
    th.stepUI();
    REQUIRE(sm->collapsedGroupsByPart[0].size() == 5);
    for (int g = 0; g < 5; ++g)
        REQUIRE(sm->isGroupCollapsed(0, g));

    th.sendToSerialization(cmsg::SetAllGroupsCollapsed({0, false}));
    th.stepUI();
    REQUIRE(sm->collapsedGroupsByPart[0].empty());
}

TEST_CASE("Fold: deleting a group below a folded group keeps the right group folded")
{
    // Bug repro: fold 3, delete group 1. Without remapping, old group 3 (now at
    // index 2) would appear unfolded and old group 4 (now at index 3) would
    // appear folded. Fix: shift higher indices down by one on delete.
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    using zad = scxt::selection::SelectionManager::ZoneAddress;

    // AddBlankZone creates group 0; makeGroups adds 4 more — total 5 groups (0..4).
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    makeGroups(th, 4); // total 5 groups: 0..4
    const auto &sm = th.engine->getSelectionManager();

    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 3, true}));
    th.stepUI();
    REQUIRE(sm->isGroupCollapsed(0, 3));

    th.sendToSerialization(cmsg::DeleteGroup(zad{0, 1, -1}));
    th.stepUI();
    REQUIRE(th.engine->getPatch()->getPart(0)->getGroups().size() == 4);
    // Old group 3 is now at index 2; its FOLDED bit must have travelled.
    REQUIRE(sm->isGroupCollapsed(0, 2));
    REQUIRE(!sm->isGroupCollapsed(0, 3));
}

TEST_CASE("Fold: delete-fixup drops the deleted group's fold bit and clamps out-of-range")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    using zad = scxt::selection::SelectionManager::ZoneAddress;

    // AddBlankZone creates group 0; makeGroups adds 4 more — total 5 groups (0..4).
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    makeGroups(th, 4); // total 5 groups: 0..4
    const auto &sm = th.engine->getSelectionManager();

    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 2, true}));
    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 4, true}));
    th.stepUI();
    REQUIRE(sm->collapsedGroupsByPart[0].size() == 2);

    // Delete the folded group 2. Group 2's bit is gone; group 4's bit shifts to 3.
    th.sendToSerialization(cmsg::DeleteGroup(zad{0, 2, -1}));
    th.stepUI();
    REQUIRE(th.engine->getPatch()->getPart(0)->getGroups().size() == 4);
    REQUIRE(!sm->isGroupCollapsed(0, 2));
    REQUIRE(sm->isGroupCollapsed(0, 3));
    REQUIRE(sm->collapsedGroupsByPart[0].size() == 1);
}

TEST_CASE("Fold: swap groups remaps fold state")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    using zad = scxt::selection::SelectionManager::ZoneAddress;

    makeGroups(th, 5); // 5 groups
    const auto &sm = th.engine->getSelectionManager();

    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 2, true}));
    th.stepUI();
    REQUIRE(sm->isGroupCollapsed(0, 2));
    REQUIRE(!sm->isGroupCollapsed(0, 4));

    INFO("Swap group 2 and group 4 (tgt.zone == -1 path)");
    th.sendToSerialization(cmsg::MoveGroupTo({zad{0, 2, -1}, zad{0, 4, -1}}));
    th.stepUI();

    REQUIRE(!sm->isGroupCollapsed(0, 2));
    REQUIRE(sm->isGroupCollapsed(0, 4));

    INFO("Swap again with both endpoints collapsed — both stay collapsed");
    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 1, true}));
    th.stepUI();
    REQUIRE(sm->isGroupCollapsed(0, 1));
    REQUIRE(sm->isGroupCollapsed(0, 4));
    th.sendToSerialization(cmsg::MoveGroupTo({zad{0, 1, -1}, zad{0, 4, -1}}));
    th.stepUI();
    REQUIRE(sm->isGroupCollapsed(0, 1));
    REQUIRE(sm->isGroupCollapsed(0, 4));
}

TEST_CASE("Fold: moveGroupToAfter remaps fold state (src<tgt)")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    using zad = scxt::selection::SelectionManager::ZoneAddress;

    makeGroups(th, 5); // 5 groups: 0..4
    const auto &sm = th.engine->getSelectionManager();

    // Initial fold state: group 2 collapsed, group 4 expanded.
    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 2, true}));
    th.stepUI();
    REQUIRE(sm->isGroupCollapsed(0, 2));
    REQUIRE(!sm->isGroupCollapsed(0, 4));

    // Move group 2 to after group 4 (tgt.zone >= 0 triggers moveAfter).
    // Layout: 0 1 2 3 4 -> 0 1 3 4 2. Original-2 (collapsed) now sits at index 4.
    // Originals at 3 and 4 shift down to 2 and 3.
    th.sendToSerialization(cmsg::MoveGroupTo({zad{0, 2, -1}, zad{0, 4, 0}}));
    th.stepUI();

    REQUIRE(!sm->isGroupCollapsed(0, 2));
    REQUIRE(!sm->isGroupCollapsed(0, 3));
    REQUIRE(sm->isGroupCollapsed(0, 4));
}

TEST_CASE("Fold: moveGroupToAfter remaps fold state (src>tgt)")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    using zad = scxt::selection::SelectionManager::ZoneAddress;

    makeGroups(th, 5); // 5 groups: 0..4
    const auto &sm = th.engine->getSelectionManager();

    // Mirrors the bug Paul reported: fold 2, then drag 2 past 4. Original-2's
    // fold state must travel with it.
    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 4, true}));
    th.stepUI();
    REQUIRE(sm->isGroupCollapsed(0, 4));
    REQUIRE(!sm->isGroupCollapsed(0, 1));

    // Move group 4 to after group 1.
    // Layout: 0 1 2 3 4 -> 0 1 4 2 3. Original-4 (collapsed) sits at index 2.
    // Originals at 2,3 shift up to 3,4.
    th.sendToSerialization(cmsg::MoveGroupTo({zad{0, 4, -1}, zad{0, 1, 0}}));
    th.stepUI();

    REQUIRE(sm->isGroupCollapsed(0, 2));
    REQUIRE(!sm->isGroupCollapsed(0, 3));
    REQUIRE(!sm->isGroupCollapsed(0, 4));
}

TEST_CASE("Fold: moveGroupToAfter preserves fold state outside the affected window")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;
    using zad = scxt::selection::SelectionManager::ZoneAddress;

    // 7 groups so we can fold indices both inside and outside the move window.
    makeGroups(th, 7);
    const auto &sm = th.engine->getSelectionManager();

    // Fold 1 (outside the move window), 2 (inside, source), and 6 (outside).
    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 1, true}));
    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 2, true}));
    th.sendToSerialization(cmsg::SetGroupCollapsed({0, 6, true}));
    th.stepUI();
    REQUIRE(sm->collapsedGroupsByPart[0].size() == 3);

    // Move 2 to after 4. Affected window [2, 4] — group 6 must NOT be lost.
    th.sendToSerialization(cmsg::MoveGroupTo({zad{0, 2, -1}, zad{0, 4, 0}}));
    th.stepUI();

    REQUIRE(sm->isGroupCollapsed(0, 1));
    REQUIRE(!sm->isGroupCollapsed(0, 2));
    REQUIRE(!sm->isGroupCollapsed(0, 3));
    REQUIRE(sm->isGroupCollapsed(0, 4));
    REQUIRE(sm->isGroupCollapsed(0, 6));
}