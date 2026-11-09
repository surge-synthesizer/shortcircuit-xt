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

namespace cmsg = scxt::messaging::client;
using SelectionManager = scxt::selection::SelectionManager;
using ZoneAddress = SelectionManager::ZoneAddress;

namespace
{
struct DeleteFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    DeleteFixture()
    {
        th.start();
        th.sync();
    }

    scxt::engine::Engine &engine() { return *th.engine; }

    template <typename T> void send(const T &msg)
    {
        th.sendToSerialization(msg);
        th.sync();
    }

    void addZones(int group, int count)
    {
        for (int z = 0; z < count; ++z)
            send(cmsg::AddBlankZone({0, group, 10 * z, 10 * z + 5, 0, 127}));
    }

    void select(int group, std::initializer_list<int> zones)
    {
        bool first{true};
        for (auto z : zones)
        {
            send(cmsg::ApplySelectActions({{0, group, z, true, first, first}}));
            first = false;
        }
    }

    void requireNothingSelected()
    {
        const auto &sm = engine().getSelectionManager();
        REQUIRE(sm->currentlySelectedZones().empty());
        REQUIRE(sm->currentlySelectedGroups().empty());
        REQUIRE(!sm->currentLeadZone(engine()).has_value());
        REQUIRE(!sm->currentLeadGroup(engine()).has_value());
    }

    SelectionManager::selectedZones_t displayGroups()
    {
        return engine().getSelectionManager()->state[0].displayGroups;
    }
};
} // namespace

TEST_CASE("Deleting the selected zones leaves nothing selected", "[delete-selection]")
{
    DeleteFixture f;
    f.addZones(0, 6);
    f.select(0, {3, 0, 4});
    REQUIRE(f.engine().getSelectionManager()->currentlySelectedZones().size() == 3);

    f.send(cmsg::DeleteAllSelectedZones(true));
    REQUIRE(f.engine().getPatch()->getPart(0)->getGroup(0)->getZones().size() == 3);
    f.requireNothingSelected();
    REQUIRE(f.displayGroups() == SelectionManager::selectedZones_t{{0, 0, -1}});
}

TEST_CASE("Deleting one zone of a selection leaves nothing selected", "[delete-selection]")
{
    DeleteFixture f;
    f.addZones(0, 6);
    f.select(0, {3, 4});

    f.send(cmsg::DeleteZone(ZoneAddress{0, 0, 1}));
    REQUIRE(f.engine().getPatch()->getPart(0)->getGroup(0)->getZones().size() == 5);
    f.requireNothingSelected();
}

TEST_CASE("Deleting a group leaves nothing selected or shown", "[delete-selection]")
{
    DeleteFixture f;
    f.addZones(0, 2);
    f.send(cmsg::CreateGroup(0));
    f.addZones(1, 2);
    f.send(cmsg::CreateGroup(0));
    f.addZones(2, 3);
    REQUIRE(f.engine().getPatch()->getPart(0)->getGroups().size() == 3);
    f.select(2, {2, 1});

    f.send(cmsg::DeleteGroup(ZoneAddress{0, 0, -1}));
    REQUIRE(f.engine().getPatch()->getPart(0)->getGroups().size() == 2);
    f.requireNothingSelected();
    REQUIRE(f.displayGroups().empty());
}
