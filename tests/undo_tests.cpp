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

#include <filesystem>
#include <string>

#include "configuration.h"
#include "console_harness.h"
#include "dsp/processor/processor.h"
#include "engine/engine.h"
#include "engine/macros.h"
#include "engine/part.h"
#include "selection/selection_manager.h"
#include "undo_manager/undo.h"

#ifndef SCXT_TEST_SOURCE_DIR
#define SCXT_TEST_SOURCE_DIR ""
#endif

namespace cmsg = scxt::messaging::client;
using ZoneAddress = scxt::selection::SelectionManager::ZoneAddress;

namespace
{

struct UndoFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    UndoFixture()
    {
        th.start();
        th.stepUI();
    }

    scxt::engine::Engine &engine() { return *th.engine; }
    scxt::undo::UndoManager &undoManager() { return th.engine->undoManager; }

    template <typename T> void send(const T &msg, size_t drainSteps = 10)
    {
        th.sendToSerialization(msg);
        th.stepUI(drainSteps);
    }

    // c2s_undo / c2s_redo. GroupChangeItem::restore hops the audio thread
    // via stopAudioThreadThenRunOnSerial so callers of sample-load undo
    // should pass a larger drainSteps.
    void sendUndo(size_t drainSteps = 10) { send(cmsg::Undo(true), drainSteps); }
    void sendRedo(size_t drainSteps = 10) { send(cmsg::Redo(true), drainSteps); }
};

} // namespace

TEST_CASE("Undo no-ops on empty stack", "[undo]")
{
    UndoFixture f;
    REQUIRE(!f.undoManager().hasUndoSteps());
    REQUIRE(!f.undoManager().hasRedoSteps());

    f.sendUndo();
    f.sendUndo();
    f.sendRedo();
    f.sendRedo();

    REQUIRE(!f.undoManager().hasUndoSteps());
    REQUIRE(!f.undoManager().hasRedoSteps());
}

TEST_CASE("Undo stack respects capacity", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));

    // Push 70 rename entries. Stack is capped at maxUndoRedoStackSize.
    for (int i = 0; i < 70; ++i)
    {
        f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, "name-" + std::to_string(i)}));
    }
    REQUIRE(f.undoManager().hasUndoSteps());

    int popped = 0;
    while (f.undoManager().hasUndoSteps() && popped < 200)
    {
        f.sendUndo();
        popped++;
    }
    REQUIRE(popped == static_cast<int>(scxt::maxUndoRedoStackSize));
    REQUIRE(!f.undoManager().hasUndoSteps());
}

TEST_CASE("Rename group undo/redo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));

    auto &part = f.engine().getPatch()->getPart(0);
    auto originalName = part->getGroup(0)->name;

    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"FooGroup"}}));
    REQUIRE(part->getGroup(0)->name == "FooGroup");
    REQUIRE(f.undoManager().hasUndoSteps());

    f.sendUndo();
    REQUIRE(part->getGroup(0)->name == originalName);
    REQUIRE(f.undoManager().hasRedoSteps());

    f.sendRedo();
    REQUIRE(part->getGroup(0)->name == "FooGroup");
}

TEST_CASE("Zone processor type change undo/redo", "[undo]")
{
    using scxt::dsp::processor::proct_CytomicSVF;
    using scxt::dsp::processor::proct_none;

    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    // AddBlankZone auto-selects the new zone; reassert for clarity.
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    REQUIRE(zone->processorStorage[0].type == proct_none);

    f.send(cmsg::SetSelectedProcessorType({true, (int32_t)0, (int32_t)proct_CytomicSVF}));
    REQUIRE(zone->processorStorage[0].type == proct_CytomicSVF);

    f.sendUndo();
    REQUIRE(zone->processorStorage[0].type == proct_none);

    f.sendRedo();
    REQUIRE(zone->processorStorage[0].type == proct_CytomicSVF);
}

TEST_CASE("Group processor type change undo/redo", "[undo]")
{
    using scxt::dsp::processor::proct_CytomicSVF;
    using scxt::dsp::processor::proct_none;

    UndoFixture f;
    // AddBlankZone selects the zone, which transitively populates
    // currentlySelectedGroups (see selection_manager.cpp:231-240), so the
    // group-scope setProcessorType path in processor_messages.h fires.
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));

    auto &group = f.engine().getPatch()->getPart(0)->getGroup(0);
    REQUIRE(group->processorStorage[0].type == proct_none);

    f.send(cmsg::SetSelectedProcessorType({false, (int32_t)0, (int32_t)proct_CytomicSVF}));
    REQUIRE(group->processorStorage[0].type == proct_CytomicSVF);

    f.sendUndo();
    REQUIRE(group->processorStorage[0].type == proct_none);

    f.sendRedo();
    REQUIRE(group->processorStorage[0].type == proct_CytomicSVF);
}

TEST_CASE("Update zone routing row undo/redo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.f));

    auto row = zone->routingTable.routes[0];
    row.depth = 0.42f;
    f.send(cmsg::UpdateZoneRoutingRow({0, row, false}));
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.42f));

    f.sendUndo();
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.f));

    f.sendRedo();
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.42f));
}

TEST_CASE("Reorder mod row undo/redo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    {
        auto r0 = zone->routingTable.routes[0];
        r0.depth = 0.25f;
        f.send(cmsg::UpdateZoneRoutingRow({0, r0, false}));
    }
    {
        auto r1 = zone->routingTable.routes[1];
        r1.depth = 0.75f;
        f.send(cmsg::UpdateZoneRoutingRow({1, r1, false}));
    }
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.25f));
    REQUIRE(zone->routingTable.routes[1].depth == Approx(0.75f));

    // Swap rows 0 and 1. ReorderModRow pushes ZoneModTableSnapshotItem.
    f.send(cmsg::ReorderModRow({true, 0, 1, false}));
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.75f));
    REQUIRE(zone->routingTable.routes[1].depth == Approx(0.25f));

    f.sendUndo();
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.25f));
    REQUIRE(zone->routingTable.routes[1].depth == Approx(0.75f));

    f.sendRedo();
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.75f));
    REQUIRE(zone->routingTable.routes[1].depth == Approx(0.25f));
}

TEST_CASE("Macro full state undo/redo", "[undo]")
{
    UndoFixture f;
    auto &macro = f.engine().getPatch()->getPart(0)->macros[0];
    auto originalName = macro.name;
    auto originalBipolar = macro.isBipolar;

    scxt::engine::Macro newMacro = macro;
    newMacro.name = "Brightness";
    newMacro.isBipolar = true;

    f.send(cmsg::SetMacroFullState({(int16_t)0, (int16_t)0, newMacro}));
    REQUIRE(macro.name == "Brightness");
    REQUIRE(macro.isBipolar == true);
    REQUIRE(f.undoManager().hasUndoSteps());

    f.sendUndo();
    REQUIRE(macro.name == originalName);
    REQUIRE(macro.isBipolar == originalBipolar);
    REQUIRE(f.undoManager().hasRedoSteps());

    f.sendRedo();
    REQUIRE(macro.name == "Brightness");
    REQUIRE(macro.isBipolar == true);
}

TEST_CASE("Load sample into group undo/redo", "[undo]")
{
    namespace fs = std::filesystem;
    auto sampleP =
        fs::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / "next" / "Beep.wav";
    INFO("sample path=" << sampleP.string());
    REQUIRE(fs::exists(sampleP));

    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    auto &part = f.engine().getPatch()->getPart(0);
    REQUIRE(part->getGroups().size() == 1);
    REQUIRE(part->getGroup(0)->getZones().size() == 1);

    // AddSampleToGroup → engine::loadSampleIntoGroup (engine.cpp:938-940).
    // Pushes GroupChangeItem before scheduling the audio-thread add.
    f.send(cmsg::AddSampleToGroup({sampleP.string(), 0, 0}), 30);
    REQUIRE(part->getGroup(0)->getZones().size() == 2);
    REQUIRE(f.undoManager().hasUndoSteps());

    // GroupChangeItem::restore uses stopAudioThreadThenRunOnSerial —
    // drain longer so the audio-thread pause completes.
    f.sendUndo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 1);
    REQUIRE(f.undoManager().hasRedoSteps());

    f.sendRedo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 2);
}
