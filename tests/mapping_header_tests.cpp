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

#include <fstream>

#include "catch2/catch2.hpp"
#include "engine/engine.h"
#include "engine/part.h"
#include "browser/browser.h"
#include "console_harness.h"
#include "messaging/client/structure_messages.h"
#include "messaging/client/selection_messages.h"
#include "messaging/client/zone_messages.h"
#include "messaging/client/interaction_messages.h"
#include "test_utils.h"

namespace cmsg = scxt::messaging::client;
using mza = scxt::engine::Engine::MidiZoneAction;

namespace
{
struct HeaderFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    HeaderFixture()
    {
        th.start();
        th.sync();
    }

    template <typename T> void send(const T &msg)
    {
        th.sendToSerialization(msg);
        th.sync();
    }

    auto &part() { return th.engine->getPatch()->getPart(0); }
    auto &sel() { return th.engine->getSelectionManager()->state[0]; }
    auto &zone(int g, int z) { return part()->getGroup(g)->getZone(z); }

    void select(int g, int z, bool distinct = true, bool lead = true)
    {
        send(cmsg::ApplySelectActions({{0, g, z, true, distinct, lead}}));
    }

    void play(int key, float vel = 0.8f)
    {
        th.sendToSerialization(cmsg::NoteFromGUI({key, vel, true}));
        th.sendToSerialization(cmsg::NoteFromGUI({key, 0.f, false}));
        th.sync();
    }

    void startAction(mza a) { send(cmsg::InitiateMidiZoneAction((int)a)); }

    int clientAction() { return th.editor->mappingHeaderState.first; }

    void undo() { send(cmsg::Undo(true)); }
};
} // namespace

TEST_CASE("Learn notes take the action and still play", "[mapping-header]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();
    addBlankZoneToGroup(part, 0, 0, 127);
    auto &grp = *part.getGroup(0);

    SECTION("key learn")
    {
        eng->midiZoneAction = mza::SET_KEY_BOUNDS_START;
        eng->processNoteOnEvent(0, 0, 60, -1, 0.8, 0.f);
        REQUIRE(eng->midiZoneAction.load() == mza::SET_KEY_BOUNDS_SECOND);
        REQUIRE(countLiveVoicesForKey(grp, 60) == 1);
        eng->processNoteOffEvent(0, 0, 60, -1, 0.f);

        eng->processNoteOnEvent(0, 0, 50, -1, 0.8, 0.f);
        REQUIRE(eng->midiZoneAction.load() == mza::NONE);
        REQUIRE(countLiveVoicesForKey(grp, 50) == 1);
    }
    SECTION("velocity learn")
    {
        eng->midiZoneAction = mza::SET_VEL_BOUNDS_START;
        eng->processNoteOnEvent(0, 0, 62, -1, 0.2, 0.f);
        eng->processNoteOnEvent(0, 0, 63, -1, 0.9, 0.f);
        REQUIRE(eng->midiZoneAction.load() == mza::NONE);
        REQUIRE(countLiveVoicesForKey(grp, 62) == 1);
        REQUIRE(countLiveVoicesForKey(grp, 63) == 1);
    }
    SECTION("select")
    {
        eng->midiZoneAction = mza::SELECT;
        eng->processNoteOnEvent(0, 0, 64, -1, 0.8, 0.f);
        REQUIRE(eng->midiZoneAction.load() == mza::SELECT);
        REQUIRE(countLiveVoicesForKey(grp, 64) == 1);
    }
}

TEST_CASE("Learn key range sets the lead zone keys", "[mapping-header]")
{
    HeaderFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 50, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 60, 70, 0, 127}));
    f.select(0, 1);
    f.select(0, 0, false, true);
    REQUIRE(f.sel().leadZone == scxt::selection::SelectionManager::ZoneAddress{0, 0, 0});

    f.startAction(mza::SET_KEY_BOUNDS_START);
    REQUIRE(f.clientAction() == (int)mza::SET_KEY_BOUNDS_START);

    SECTION("in order")
    {
        f.play(36);
        f.play(48);
        REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyStart == 36);
        REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyEnd == 48);
    }
    SECTION("reversed, which sorts")
    {
        f.play(52);
        f.play(30);
        REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyStart == 30);
        REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyEnd == 52);
    }

    // the button turns itself off, the other selected zone is untouched, and it is one undo
    REQUIRE(f.th.engine->midiZoneAction.load() == mza::NONE);
    REQUIRE(f.clientAction() == (int)mza::NONE);
    REQUIRE(f.zone(0, 1)->mapping.keyboardRange.keyStart == 60);
    REQUIRE(f.zone(0, 1)->mapping.keyboardRange.keyEnd == 70);

    f.undo();
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyStart == 40);
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyEnd == 50);

    // after the learn the next note plays rather than learning
    f.play(45);
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyStart == 40);
}

TEST_CASE("Learn key range lands on the transposed keys", "[mapping-header]")
{
    HeaderFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 50, 0, 127}));
    f.select(0, 0);
    f.part()->configuration.transpose = 12;

    f.startAction(mza::SET_KEY_BOUNDS_START);
    f.play(36);
    f.play(48);
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyStart == 48);
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyEnd == 60);
}

TEST_CASE("Learn velocity range sets the lead zone velocities", "[mapping-header]")
{
    HeaderFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 50, 0, 127}));
    f.select(0, 0);
    f.zone(0, 0)->mapping.velocityRange.fadeStart = 40;
    f.zone(0, 0)->mapping.velocityRange.fadeEnd = 40;

    f.startAction(mza::SET_VEL_BOUNDS_START);
    REQUIRE(f.clientAction() == (int)mza::SET_VEL_BOUNDS_START);
    f.play(60, 96 / 127.f);
    f.play(61, 32 / 127.f);

    const auto &vr = f.zone(0, 0)->mapping.velocityRange;
    REQUIRE(vr.velStart == 32);
    REQUIRE(vr.velEnd == 96);
    // the fades shrink to fit the new range
    REQUIRE(vr.fadeStart + vr.fadeEnd <= vr.velEnd - vr.velStart + 1);
    REQUIRE(f.clientAction() == (int)mza::NONE);
    // keys are left alone
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyStart == 40);
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyEnd == 50);
}

TEST_CASE("Turning a learn off lets the next note play", "[mapping-header]")
{
    HeaderFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 50, 0, 127}));
    f.select(0, 0);

    f.startAction(mza::SET_KEY_BOUNDS_START);
    f.startAction(mza::NONE);
    REQUIRE(f.clientAction() == (int)mza::NONE);
    f.play(36);
    f.play(48);
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyStart == 40);
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyEnd == 50);
}

TEST_CASE("MIDI select picks the zone a note lands in", "[mapping-header]")
{
    HeaderFixture f;
    // two velocity layers on the low keys, one zone above, and a hidden group
    f.send(cmsg::AddBlankZone({0, 0, 40, 50, 0, 63}));
    f.send(cmsg::AddBlankZone({0, 0, 40, 50, 64, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 60, 70, 0, 127}));
    f.send(cmsg::CreateGroup(0));
    f.send(cmsg::AddBlankZone({0, 1, 80, 90, 0, 127}));
    f.select(0, 2);

    f.startAction(mza::SELECT);
    REQUIRE(f.clientAction() == (int)mza::SELECT);

    f.play(45, 0.9f);
    REQUIRE(f.sel().leadZone == scxt::selection::SelectionManager::ZoneAddress{0, 0, 1});
    REQUIRE(f.sel().selectedZones.size() == 1);

    f.play(45, 0.2f);
    REQUIRE(f.sel().leadZone == scxt::selection::SelectionManager::ZoneAddress{0, 0, 0});

    // a group with nothing selected isn't showing, so its zones aren't candidates
    f.play(85);
    REQUIRE(f.sel().leadZone == scxt::selection::SelectionManager::ZoneAddress{0, 0, 0});

    // a note no zone answers leaves the selection alone
    f.play(100);
    REQUIRE(f.sel().leadZone == scxt::selection::SelectionManager::ZoneAddress{0, 0, 0});

    // it stays on until turned off
    REQUIRE(f.th.engine->midiZoneAction.load() == mza::SELECT);
    f.startAction(mza::NONE);
    f.play(65);
    REQUIRE(f.sel().leadZone == scxt::selection::SelectionManager::ZoneAddress{0, 0, 0});
}

TEST_CASE("MIDI select keeps the groups it started with", "[mapping-header]")
{
    HeaderFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 50, 0, 127}));
    f.send(cmsg::CreateGroup(0));
    f.send(cmsg::AddBlankZone({0, 1, 60, 70, 0, 127}));
    f.select(0, 0);
    f.select(1, 0, false, false);

    f.startAction(mza::SELECT);
    f.play(65);
    REQUIRE(f.sel().leadZone == scxt::selection::SelectionManager::ZoneAddress{0, 1, 0});

    // group 0 no longer has a selected zone, but it was showing when select went on
    f.play(45);
    REQUIRE(f.sel().leadZone == scxt::selection::SelectionManager::ZoneAddress{0, 0, 0});
}

TEST_CASE("MIDI select with nothing selected can reach any group", "[mapping-header]")
{
    HeaderFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 50, 0, 127}));
    f.send(cmsg::CreateGroup(0));
    f.send(cmsg::AddBlankZone({0, 1, 60, 70, 0, 127}));
    f.send(cmsg::ApplySelectActions(
        {scxt::selection::SelectionManager::SelectActionContents::deselectSentinel()}));
    REQUIRE(f.sel().selectedZones.empty());

    f.startAction(mza::SELECT);
    f.play(65);
    REQUIRE(f.sel().leadZone == scxt::selection::SelectionManager::ZoneAddress{0, 1, 0});

    // group 0 is still reachable though only group 1 now shows
    f.play(45);
    REQUIRE(f.sel().leadZone == scxt::selection::SelectionManager::ZoneAddress{0, 0, 0});
}

TEST_CASE("Zone solo follows the selection", "[mapping-header]")
{
    HeaderFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 60, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 40, 60, 0, 127}));
    f.select(0, 1);

    // blank zones end their voices at once under a running engine, so read the flags the
    // audio thread plays from; the next test shows the voices obey them
    auto soloed = [&f]() {
        std::array<bool, 3> res{f.part()->zoneSoloActive, f.zone(0, 0)->soloedBySelection,
                                f.zone(0, 1)->soloedBySelection};
        return res;
    };

    REQUIRE(soloed() == std::array<bool, 3>{false, false, false});

    f.send(cmsg::SetZoneSolo(true));
    REQUIRE(f.th.editor->mappingHeaderState.second);
    REQUIRE(soloed() == std::array<bool, 3>{true, false, true});

    f.select(0, 0);
    REQUIRE(soloed() == std::array<bool, 3>{true, true, false});

    // with nothing selected there is nothing to solo
    f.send(cmsg::ApplySelectActions(
        {scxt::selection::SelectionManager::SelectActionContents::deselectSentinel()}));
    REQUIRE(f.sel().selectedZones.empty());
    REQUIRE(!f.part()->zoneSoloActive);

    f.select(0, 0);
    f.select(0, 1, false, false);
    REQUIRE(soloed() == std::array<bool, 3>{true, true, true});

    f.send(cmsg::SetZoneSolo(false));
    REQUIRE(!f.th.editor->mappingHeaderState.second);
    REQUIRE(soloed() == std::array<bool, 3>{false, false, false});
}

TEST_CASE("Zone solo flags keep unselected zones from making voices", "[mapping-header]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();
    addBlankZoneToGroup(part, 0, 40, 60);
    addBlankZoneToGroup(part, 0, 40, 60);

    // the flags the serialization thread pushes down
    part.zoneSoloActive = true;
    part.getGroup(0)->getZone(1)->soloedBySelection = true;
    eng->processNoteOnEvent(0, 0, 50, -1, 0.8, 0.f);
    REQUIRE(countLiveVoicesForKey(*part.getGroup(0), 50) == 1);
    REQUIRE(part.getGroup(0)->getZone(1)->activeVoices == 1);
    REQUIRE(part.getGroup(0)->getZone(0)->activeVoices == 0);
}

TEST_CASE("Fix overlaps splits partial overlaps down the middle", "[mapping-header]")
{
    HeaderFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 52, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    // fully inside the first, so left alone
    f.send(cmsg::AddBlankZone({0, 0, 42, 44, 0, 127}));
    // same keys as the second, overlapping velocities
    f.send(cmsg::AddBlankZone({0, 0, 70, 80, 0, 80}));
    f.send(cmsg::AddBlankZone({0, 0, 70, 80, 60, 127}));
    f.select(0, 0);
    for (int z = 1; z < 5; ++z)
        f.select(0, z, false, false);

    f.send(cmsg::ApplyZoneBatchOp({{}, scxt::engine::Engine::FIX_OVERLAPS}));
    const auto &k0 = f.zone(0, 0)->mapping.keyboardRange;
    const auto &k1 = f.zone(0, 1)->mapping.keyboardRange;
    REQUIRE(k0.keyStart == 40);
    REQUIRE(k0.keyEnd == 50);
    REQUIRE(k1.keyStart == 51);
    REQUIRE(k1.keyEnd == 60);
    REQUIRE(f.zone(0, 2)->mapping.keyboardRange.keyStart == 42);
    REQUIRE(f.zone(0, 2)->mapping.keyboardRange.keyEnd == 44);

    const auto &v3 = f.zone(0, 3)->mapping.velocityRange;
    const auto &v4 = f.zone(0, 4)->mapping.velocityRange;
    REQUIRE(v3.velEnd == 70);
    REQUIRE(v4.velStart == 71);
    REQUIRE(f.zone(0, 3)->mapping.keyboardRange.keyStart == 70);

    f.undo();
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyEnd == 52);
    REQUIRE(f.zone(0, 1)->mapping.keyboardRange.keyStart == 48);
    REQUIRE(f.zone(0, 4)->mapping.velocityRange.velStart == 60);
}

TEST_CASE("Fade overlaps crossfades both keys and velocities", "[mapping-header]")
{
    HeaderFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 40, 52, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 70, 80, 0, 80}));
    f.send(cmsg::AddBlankZone({0, 0, 70, 80, 60, 127}));
    f.select(0, 0);
    for (int z = 1; z < 4; ++z)
        f.select(0, z, false, false);

    f.send(cmsg::ApplyZoneBatchOp({{}, scxt::engine::Engine::AUTO_CROSSFADES}));
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.fadeEnd == 5);
    REQUIRE(f.zone(0, 1)->mapping.keyboardRange.fadeStart == 5);
    REQUIRE(f.zone(0, 2)->mapping.velocityRange.fadeEnd == 21);
    REQUIRE(f.zone(0, 3)->mapping.velocityRange.fadeStart == 21);
    // the ranges stay put
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.keyEnd == 52);

    // one undo takes back both
    f.undo();
    REQUIRE(f.zone(0, 0)->mapping.keyboardRange.fadeEnd == 0);
    REQUIRE(f.zone(0, 2)->mapping.velocityRange.fadeEnd == 0);
}

TEST_CASE("The header state reaches a fresh client", "[mapping-header]")
{
    HeaderFixture f;
    f.startAction(mza::SELECT);
    f.send(cmsg::SetZoneSolo(true));
    f.th.editor->mappingHeaderState = {-1, false};
    // what a reopened editor sends
    f.send(cmsg::RegisterClient(true));
    REQUIRE(f.th.editor->mappingHeaderState.first == (int)mza::SELECT);
    REQUIRE(f.th.editor->mappingHeaderState.second);
}

TEST_CASE("Sample folders list single samples in natural order", "[mapping-header]")
{
    auto dir = fs::temp_directory_path() / "scxt-mapping-header-folder-test";
    fs::remove_all(dir);
    fs::create_directories(dir / "sub");
    for (auto n : {"s10.wav", "S1.wav", "s2.flac", "notes.txt", "kit.sf2", "sub/s3.wav"})
        std::ofstream(dir / n) << "x";

    auto files = scxt::browser::Browser::singleSamplesIn(dir);
    REQUIRE(files.size() == 3);
    REQUIRE(files[0].filename() == "S1.wav");
    REQUIRE(files[1].filename() == "s2.flac");
    REQUIRE(files[2].filename() == "s10.wav");

    // the arrows wrap at both ends
    REQUIRE(scxt::browser::Browser::stepPatchFile(files, files[2], 1) == files[0]);
    REQUIRE(scxt::browser::Browser::stepPatchFile(files, files[0], -1) == files[2]);

    REQUIRE(scxt::browser::Browser::singleSamplesIn(dir / "missing").empty());
    fs::remove_all(dir);
}
