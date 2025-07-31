/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "catch2/catch2.hpp"
#include "engine/engine.h"

#include "test_utilities.h"
#include "console_ui.h"

TEST_CASE("Create a Single Blank Zone and it is Selected")
{
    scxt::tests::TestHarness th;
    th.start();

    th.stepUI();
    th.sendToSerialization(scxt::messaging::client::AddBlankZone({0, 0, 60, 72, 0, 64}));
    th.stepUI();

    REQUIRE(th.engine->getSelectionManager()->leadZone[0] ==
            scxt::selection::SelectionManager::ZoneAddress{0, 0, 0});
}

TEST_CASE("Create five zones then multi-select")
{
    scxt::tests::TestHarness th;
    th.start();
    th.stepUI();

    namespace cmsg = scxt::messaging::client;

    using abz = cmsg::AddBlankZone;
    using zad = scxt::selection::SelectionManager::ZoneAddress;
    using asa = cmsg::ApplySelectAction;
    using sac = scxt::selection::SelectionManager::SelectActionContents;

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
    th.sendToSerialization(asa(sac{0, 0, 2, true, true, true}));
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 2});
    REQUIRE(sm->allSelectedZones[0].size() == 1);

    INFO("Single select a second zone not as lead");
    th.sendToSerialization(asa(sac{0, 0, 0, true, false, false}));
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 2});
    REQUIRE(sm->allSelectedZones[0].size() == 2);

    INFO("Swap the lead");
    th.sendToSerialization(asa(sac{0, 0, 0, true, false, true}));
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 0});
    REQUIRE(sm->allSelectedZones[0].size() == 2);

    INFO("Unselect the non-lead");
    th.sendToSerialization(asa(sac{0, 0, 2, false, false, false}));
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 0});
    REQUIRE(sm->allSelectedZones[0].size() == 1);

    INFO("Select two more groups as non-lead and lead");
    th.sendToSerialization(asa(sac{0, 0, 1, true, false, true}));
    th.sendToSerialization(asa(sac{0, 0, 3, true, false, false}));
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 1});
    REQUIRE(sm->allSelectedZones[0].size() == 3);

    INFO("Select a different zone disticnt and get it lead with set clear");
    th.sendToSerialization(asa(sac{0, 0, 4, true, true, true}));
    th.stepUI();
    REQUIRE(sm->leadZone[0] == zad{0, 0, 4});
    REQUIRE(sm->allSelectedZones[0].size() == 1);
}