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
#include "engine/part.h"
#include "console_harness.h"
#include "messaging/client/structure_messages.h"
#include "messaging/client/selection_messages.h"
#include "json/stream.h"
#include "test_utils.h"

namespace cmsg = scxt::messaging::client;
using ZoneAddress = scxt::selection::SelectionManager::ZoneAddress;

namespace
{
struct Fixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    Fixture()
    {
        th.start();
        th.stepUI();
    }

    template <typename T> void send(const T &msg, size_t drain = 20)
    {
        th.sendToSerialization(msg);
        th.stepUI(drain);
    }

    auto &part() { return th.engine->getPatch()->getPart(0); }
    auto &sel() { return th.engine->getSelectionManager()->state[0]; }

    // zones are told apart by their first key
    void addZone(int group, int key) { send(cmsg::AddBlankZone({0, group, key, key, 0, 127})); }

    std::vector<int> keysIn(int group)
    {
        std::vector<int> res;
        for (const auto &z : *part()->getGroup(group))
            res.push_back(z->mapping.keyboardRange.keyStart);
        return res;
    }

    ZoneAddress addrOfKey(int key)
    {
        for (int g = 0; g < (int)part()->getGroups().size(); ++g)
            for (int z = 0; z < (int)part()->getGroup(g)->getZones().size(); ++z)
                if (part()->getGroup(g)->getZone(z)->mapping.keyboardRange.keyStart == key)
                    return {0, g, z};
        return {};
    }

    std::vector<std::string> groupNames()
    {
        std::vector<std::string> res;
        for (const auto &g : *part())
            res.push_back(g->name);
        return res;
    }

    void select(const ZoneAddress &a, bool distinct = true, bool lead = true)
    {
        send(cmsg::ApplySelectActions({{a.part, a.group, a.zone, true, distinct, lead}}));
    }

    std::set<int> selectedKeys()
    {
        std::set<int> res;
        for (const auto &a : sel().selectedZones)
            res.insert(part()->getGroup(a.group)->getZone(a.zone)->mapping.keyboardRange.keyStart);
        return res;
    }
    int leadKey()
    {
        auto a = sel().leadZone;
        return part()->getGroup(a.group)->getZone(a.zone)->mapping.keyboardRange.keyStart;
    }

    void undo() { send(cmsg::Undo(true), 40); }
};
} // namespace

TEST_CASE("Move zones lands them before the target", "[sidebar]")
{
    Fixture f;
    for (int k : {40, 41, 42, 43})
        f.addZone(0, k);
    REQUIRE(f.keysIn(0) == std::vector<int>{40, 41, 42, 43});

    SECTION("one zone up")
    {
        f.send(cmsg::MoveZonesTo({{f.addrOfKey(42)}, {0, 0, 0}, false}));
        REQUIRE(f.keysIn(0) == std::vector<int>{42, 40, 41, 43});
    }
    SECTION("one zone down")
    {
        f.send(cmsg::MoveZonesTo({{f.addrOfKey(40)}, {0, 0, 3}, false}));
        REQUIRE(f.keysIn(0) == std::vector<int>{41, 42, 40, 43});
    }
    SECTION("to the end of the group")
    {
        f.send(cmsg::MoveZonesTo({{f.addrOfKey(40)}, {0, 0, -1}, false}));
        REQUIRE(f.keysIn(0) == std::vector<int>{41, 42, 43, 40});
    }
    SECTION("a gapped selection lands contiguous")
    {
        f.send(cmsg::MoveZonesTo({{f.addrOfKey(40), f.addrOfKey(42)}, {0, 0, -1}, false}));
        REQUIRE(f.keysIn(0) == std::vector<int>{41, 43, 40, 42});
    }
    SECTION("before a zone which is itself moving")
    {
        f.send(cmsg::MoveZonesTo({{f.addrOfKey(41), f.addrOfKey(43)}, {0, 0, 1}, false}));
        REQUIRE(f.keysIn(0) == std::vector<int>{40, 41, 43, 42});
    }
    SECTION("undo puts them back")
    {
        f.send(cmsg::MoveZonesTo({{f.addrOfKey(42)}, {0, 0, 0}, false}));
        f.undo();
        REQUIRE(f.keysIn(0) == std::vector<int>{40, 41, 42, 43});
    }
}

TEST_CASE("Move zones across groups and into a new one", "[sidebar]")
{
    Fixture f;
    f.addZone(0, 40);
    f.addZone(0, 41);
    f.send(cmsg::CreateGroup(0));
    f.addZone(1, 50);
    f.addZone(1, 51);

    SECTION("into another group mid way")
    {
        f.send(cmsg::MoveZonesTo({{f.addrOfKey(40)}, {0, 1, 1}, false}));
        REQUIRE(f.keysIn(0) == std::vector<int>{41});
        REQUIRE(f.keysIn(1) == std::vector<int>{50, 40, 51});
    }
    SECTION("into a new group")
    {
        f.send(cmsg::MoveZonesTo({{f.addrOfKey(41), f.addrOfKey(50)}, {0, -1, -1}, false}));
        REQUIRE(f.part()->getGroups().size() == 3);
        REQUIRE(f.keysIn(2) == std::vector<int>{41, 50});
        REQUIRE(f.part()->getGroup(2)->name.find("New Group") == 0);
    }
}

TEST_CASE("Moved zones keep their selection", "[sidebar]")
{
    Fixture f;
    for (int k : {40, 41, 42, 43})
        f.addZone(0, k);
    f.select(f.addrOfKey(41));
    f.select(f.addrOfKey(43), false, false);
    REQUIRE(f.selectedKeys() == std::set<int>{41, 43});

    // a lone zone dragged out from under the selection leaves the selection alone
    f.send(cmsg::MoveZonesTo({{f.addrOfKey(40)}, {0, 0, -1}, false}));
    REQUIRE(f.keysIn(0) == std::vector<int>{41, 42, 43, 40});
    REQUIRE(f.selectedKeys() == std::set<int>{41, 43});
    REQUIRE(f.leadKey() == 41);

    f.send(cmsg::MoveZonesTo({{f.addrOfKey(41), f.addrOfKey(43)}, {0, -1, -1}, false}));
    REQUIRE(f.selectedKeys() == std::set<int>{41, 43});
    REQUIRE(f.leadKey() == 41);
    REQUIRE(f.sel().leadZone.group == 1);
}

TEST_CASE("Copying zones leaves the originals and selects the copies", "[sidebar]")
{
    Fixture f;
    for (int k : {40, 41, 42})
        f.addZone(0, k);
    f.send(cmsg::MoveZonesTo({{f.addrOfKey(40), f.addrOfKey(42)}, {0, 0, 1}, true}));
    REQUIRE(f.keysIn(0) == std::vector<int>{40, 40, 42, 41, 42});
    REQUIRE(f.sel().selectedZones.size() == 2);
    REQUIRE(f.sel().selectedZones.count({0, 0, 1}) == 1);
    REQUIRE(f.sel().selectedZones.count({0, 0, 2}) == 1);
    REQUIRE(f.part()->getGroup(0)->getZone(1)->getName() !=
            f.part()->getGroup(0)->getZone(0)->getName());

    f.undo();
    REQUIRE(f.keysIn(0) == std::vector<int>{40, 41, 42});
}

TEST_CASE("Move groups lands them before the target", "[sidebar]")
{
    Fixture f;
    for (int g = 0; g < 4; ++g)
    {
        f.addZone(g, 40 + g);
        f.send(cmsg::RenameGroup({{0, g, -1}, "G" + std::to_string(g)}));
    }
    REQUIRE(f.groupNames() == std::vector<std::string>{"G0", "G1", "G2", "G3"});

    SECTION("single")
    {
        f.send(cmsg::MoveGroupsTo({0, {3}, 1, false}));
        REQUIRE(f.groupNames() == std::vector<std::string>{"G0", "G3", "G1", "G2"});
    }
    SECTION("gapped to the end")
    {
        f.send(cmsg::MoveGroupsTo({0, {0, 2}, -1, false}));
        REQUIRE(f.groupNames() == std::vector<std::string>{"G1", "G3", "G0", "G2"});
    }
    SECTION("folds and selection follow the groups")
    {
        f.send(cmsg::SetGroupCollapsed({0, 0, true}));
        f.select(f.addrOfKey(40));
        f.send(cmsg::MoveGroupsTo({0, {0}, -1, false}));
        REQUIRE(f.groupNames() == std::vector<std::string>{"G1", "G2", "G3", "G0"});
        REQUIRE(f.th.engine->getSelectionManager()->isGroupCollapsed(0, 3));
        REQUIRE(!f.th.engine->getSelectionManager()->isGroupCollapsed(0, 0));
        REQUIRE(f.sel().leadZone == ZoneAddress{0, 3, 0});
        REQUIRE(f.sel().leadGroup == ZoneAddress{0, 3, -1});
    }
    SECTION("copy")
    {
        f.send(cmsg::MoveGroupsTo({0, {1, 2}, 0, true}));
        REQUIRE(f.groupNames() ==
                std::vector<std::string>{"G1 (copy)", "G2 (copy)", "G0", "G1", "G2", "G3"});
        REQUIRE(f.keysIn(0) == std::vector<int>{41});
        REQUIRE(f.sel().selectedGroups.count({0, 0, -1}) == 1);
        REQUIRE(f.sel().selectedGroups.count({0, 1, -1}) == 1);
        f.undo();
        REQUIRE(f.groupNames() == std::vector<std::string>{"G0", "G1", "G2", "G3"});
    }
}

TEST_CASE("Move zones to new groups", "[sidebar]")
{
    Fixture f;
    for (int k : {40, 41, 42})
        f.addZone(0, k);
    f.send(cmsg::RenameGroup({{0, 0, -1}, "Source"}));
    f.part()->getGroup(0)->outputInfo.amplitude = 0.5f;

    SECTION("one empty group")
    {
        f.send(cmsg::MoveZonesToNewGroups({{f.addrOfKey(40), f.addrOfKey(42)}, false, false}));
        REQUIRE(f.part()->getGroups().size() == 2);
        REQUIRE(f.keysIn(1) == std::vector<int>{40, 42});
        REQUIRE(f.part()->getGroup(1)->outputInfo.amplitude == 1.f);
    }
    SECTION("each its own clone")
    {
        f.select(f.addrOfKey(40));
        f.select(f.addrOfKey(41), false, false);
        f.send(cmsg::MoveZonesToNewGroups({{}, true, true}));
        REQUIRE(f.part()->getGroups().size() == 3);
        REQUIRE(f.keysIn(0) == std::vector<int>{42});
        REQUIRE(f.keysIn(1) == std::vector<int>{40});
        REQUIRE(f.keysIn(2) == std::vector<int>{41});
        REQUIRE(f.part()->getGroup(1)->outputInfo.amplitude == 0.5f);
        REQUIRE(f.part()->getGroup(1)->name == f.part()->getGroup(1)->getZone(0)->getName());
        REQUIRE(f.selectedKeys() == std::set<int>{40, 41});
    }
    SECTION("one clone")
    {
        f.send(cmsg::MoveZonesToNewGroups({{f.addrOfKey(41)}, false, true}));
        REQUIRE(f.part()->getGroup(1)->name == "Source (copy)");
        REQUIRE(f.part()->getGroup(1)->outputInfo.amplitude == 0.5f);
    }
}

TEST_CASE("Zone batch operations", "[sidebar]")
{
    Fixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 52, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 70, 80, 0, 127}));
    f.select({0, 0, 0});
    f.select({0, 0, 1}, false, false);
    f.select({0, 0, 2}, false, false);
    auto &g = f.part()->getGroup(0);

    SECTION("root keys")
    {
        f.send(cmsg::ApplyZoneBatchOp({{}, scxt::engine::Engine::ROOT_TO_FIRST_KEY}));
        REQUIRE(g->getZone(0)->mapping.rootKey == 40);
        f.send(cmsg::ApplyZoneBatchOp({{}, scxt::engine::Engine::ROOT_TO_LAST_KEY}));
        REQUIRE(g->getZone(1)->mapping.rootKey == 60);
        f.send(cmsg::ApplyZoneBatchOp({{}, scxt::engine::Engine::ROOT_TO_CENTER_KEY}));
        REQUIRE(g->getZone(2)->mapping.rootKey == 75);
        f.undo();
        REQUIRE(g->getZone(2)->mapping.rootKey == 80);
    }
    SECTION("key crossfades across the overlap only")
    {
        f.send(cmsg::ApplyZoneBatchOp({{}, scxt::engine::Engine::AUTO_KEY_CROSSFADES}));
        REQUIRE(g->getZone(0)->mapping.keyboardRange.fadeEnd == 5);
        REQUIRE(g->getZone(0)->mapping.keyboardRange.fadeStart == 0);
        REQUIRE(g->getZone(1)->mapping.keyboardRange.fadeStart == 5);
        REQUIRE(g->getZone(1)->mapping.keyboardRange.fadeEnd == 0);
        REQUIRE(g->getZone(2)->mapping.keyboardRange.fadeStart == 0);

        f.send(cmsg::ApplyZoneBatchOp({{}, scxt::engine::Engine::REMOVE_KEY_CROSSFADES}));
        REQUIRE(g->getZone(0)->mapping.keyboardRange.fadeEnd == 0);
        REQUIRE(g->getZone(1)->mapping.keyboardRange.fadeStart == 0);
    }
    SECTION("velocity crossfades")
    {
        g->getZone(0)->mapping.velocityRange = {0, 80};
        g->getZone(1)->mapping.velocityRange = {60, 127};
        f.send(cmsg::ApplyZoneBatchOp({{}, scxt::engine::Engine::AUTO_VELOCITY_CROSSFADES}));
        REQUIRE(g->getZone(0)->mapping.velocityRange.fadeEnd == 21);
        REQUIRE(g->getZone(1)->mapping.velocityRange.fadeStart == 21);
    }
}

TEST_CASE("Linked groups select their zones together", "[sidebar]")
{
    Fixture f;
    for (int k : {40, 41, 42})
        f.addZone(0, k);
    f.send(cmsg::CreateGroup(0));
    f.addZone(1, 50);
    f.select(f.addrOfKey(50));

    f.send(cmsg::SetLinkZoneSelection({{{0, 0, -1}}, true}));
    REQUIRE(f.part()->getGroup(0)->linkZoneSelection);

    f.select(f.addrOfKey(41));
    REQUIRE(f.selectedKeys() == std::set<int>{40, 41, 42});
    REQUIRE(f.leadKey() == 40);

    // alt style, adding as lead, keeps the zone picked
    f.select(f.addrOfKey(42), false, true);
    REQUIRE(f.leadKey() == 42);

    // deselecting one lets go of them all
    f.select(f.addrOfKey(50), false, true);
    REQUIRE(f.selectedKeys() == std::set<int>{40, 41, 42, 50});
    f.send(cmsg::ApplySelectActions({{0, 0, 0, false, false, false}}));
    REQUIRE(f.selectedKeys() == std::set<int>{50});

    SECTION("streams")
    {
        auto js = scxt::json::streamPatch(*f.th.engine->getPatch());
        REQUIRE(js.find("\"lzs\"") != std::string::npos);
    }
    SECTION("undo unlinks")
    {
        f.undo();
        REQUIRE(!f.part()->getGroup(0)->linkZoneSelection);
    }
}

TEST_CASE("Deselecting a group lets go of its zones", "[sidebar]")
{
    Fixture f;
    f.addZone(0, 40);
    f.send(cmsg::CreateGroup(0));
    f.addZone(1, 50);
    f.select(f.addrOfKey(40));
    f.select(f.addrOfKey(50), false, false);
    REQUIRE(f.selectedKeys() == std::set<int>{40, 50});

    auto se = scxt::selection::SelectionManager::SelectActionContents(ZoneAddress{0, 1, -1}, false,
                                                                      false, false);
    se.forZone = false;
    f.send(cmsg::ApplySelectActions({se}));
    REQUIRE(f.selectedKeys() == std::set<int>{40});
    REQUIRE(f.sel().selectedGroups.count({0, 1, -1}) == 0);
}

TEST_CASE("A group added without the lead keeps the lead", "[sidebar]")
{
    Fixture f;
    f.addZone(0, 40);
    f.send(cmsg::CreateGroup(0));
    f.addZone(1, 50);
    using sac_t = scxt::selection::SelectionManager::SelectActionContents;
    auto g0 = sac_t(ZoneAddress{0, 0, -1}, true, true, true);
    auto g1 = sac_t(ZoneAddress{0, 1, -1}, true, false, false);
    g0.forZone = g1.forZone = false;
    f.send(cmsg::ApplySelectActions({g0}));
    f.send(cmsg::ApplySelectActions({g1}));
    REQUIRE(f.selectedKeys() == std::set<int>{40, 50});
    REQUIRE(f.leadKey() == 40);
    REQUIRE(f.sel().leadGroup == ZoneAddress{0, 0, -1});
}

TEST_CASE("Paste group without its zones", "[sidebar]")
{
    Fixture f;
    f.addZone(0, 40);
    f.send(cmsg::CopyGroup({0, 0, -1}));
    f.send(cmsg::PasteGroupWithoutZones({0, 0, -1}));
    REQUIRE(f.part()->getGroups().size() == 2);
    REQUIRE(f.part()->getGroup(1)->getZones().empty());
}

TEST_CASE("Initialize and duplicate a part", "[sidebar]")
{
    Fixture f;
    f.addZone(0, 40);
    f.addZone(0, 41);
    f.part()->macros[0].name = "Changed";

    SECTION("duplicate goes to the first free slot")
    {
        f.send(cmsg::DuplicatePart(0), 60);
        auto &p1 = f.th.engine->getPatch()->getPart(1);
        REQUIRE(p1->configuration.active);
        REQUIRE(p1->getGroups().size() == 1);
        REQUIRE(p1->getGroup(0)->getZones().size() == 2);
        REQUIRE(p1->macros[0].name == "Changed");
        REQUIRE(f.part()->getGroup(0)->getZones().size() == 2);
    }
    SECTION("initialize clears the slot but keeps it on")
    {
        auto ch = f.part()->configuration.channel;
        f.send(cmsg::InitializePart(0), 60);
        REQUIRE(f.part()->getGroups().empty());
        REQUIRE(f.part()->macros[0].name != "Changed");
        REQUIRE(f.part()->configuration.active);
        REQUIRE(f.part()->configuration.channel == ch);

        f.undo();
        REQUIRE(f.part()->getGroup(0)->getZones().size() == 2);
    }
}
