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
#include "json/stream.h"
#include "patch_io/patch_io.h"
#include "selection/selection_manager.h"
#include "undo_manager/payload_undoable_items.h"
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

TEST_CASE("Zone mapping gesture coalesces to one undo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto origRootKey = zone->mapping.rootKey;
    auto baseSize = f.undoManager().undoStackSize();

    f.send(cmsg::BeginEdit({(int32_t)cmsg::EditSubtree::zone_mapping, true, -1}));
    REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

    for (int i = 0; i < 20; ++i)
    {
        f.send(cmsg::UpdateZoneMappingInt16TValue(
            {offsetof(scxt::engine::Zone::ZoneMappingData, rootKey),
             (int16_t)(origRootKey + 1 + i)}));
    }
    REQUIRE(zone->mapping.rootKey == origRootKey + 20);
    REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

    f.sendUndo();
    REQUIRE(zone->mapping.rootKey == origRootKey);

    f.sendRedo();
    REQUIRE(zone->mapping.rootKey == origRootKey + 20);
}

TEST_CASE("Output gesture coalesces to one undo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    SECTION("zone output")
    {
        auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
        auto origAmp = zone->outputInfo.amplitude;
        auto baseSize = f.undoManager().undoStackSize();

        f.send(cmsg::BeginEdit({(int32_t)cmsg::EditSubtree::output_info, true, -1}));
        REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

        for (int i = 0; i < 20; ++i)
        {
            f.send(cmsg::UpdateZoneOutputFloatValue(
                {offsetof(scxt::engine::Zone::ZoneOutputInfo, amplitude), 0.2f + 0.01f * i}));
        }
        REQUIRE(zone->outputInfo.amplitude == Approx(0.39f));
        REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

        f.sendUndo();
        REQUIRE(zone->outputInfo.amplitude == Approx(origAmp));

        f.sendRedo();
        REQUIRE(zone->outputInfo.amplitude == Approx(0.39f));
    }

    SECTION("group output")
    {
        auto &group = f.engine().getPatch()->getPart(0)->getGroup(0);
        auto origAmp = group->outputInfo.amplitude;
        auto baseSize = f.undoManager().undoStackSize();

        f.send(cmsg::BeginEdit({(int32_t)cmsg::EditSubtree::output_info, false, -1}));
        REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

        for (int i = 0; i < 20; ++i)
        {
            f.send(cmsg::UpdateGroupOutputFloatValue(
                {offsetof(scxt::engine::Group::GroupOutputInfo, amplitude), 0.2f + 0.01f * i}));
        }
        REQUIRE(group->outputInfo.amplitude == Approx(0.39f));
        REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

        f.sendUndo();
        REQUIRE(group->outputInfo.amplitude == Approx(origAmp));

        f.sendRedo();
        REQUIRE(group->outputInfo.amplitude == Approx(0.39f));
    }
}

TEST_CASE("Rename zone undo/redo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto origName = zone->givenName;

    f.send(cmsg::RenameZone({ZoneAddress{0, 0, 0}, std::string{"FooZone"}}));
    REQUIRE(zone->givenName == "FooZone");

    f.sendUndo();
    REQUIRE(zone->givenName == origName);

    f.sendRedo();
    REQUIRE(zone->givenName == "FooZone");
}

TEST_CASE("Zone output int16 undo/redo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto orig = (int16_t)zone->outputInfo.routeTo;
    auto next = (int16_t)(orig + 1);

    f.send(cmsg::UpdateZoneOutputInt16TValue(
        {offsetof(scxt::engine::Zone::ZoneOutputInfo, routeTo), next}));
    REQUIRE((int16_t)zone->outputInfo.routeTo == next);

    f.sendUndo();
    REQUIRE((int16_t)zone->outputInfo.routeTo == orig);

    f.sendRedo();
    REQUIRE((int16_t)zone->outputInfo.routeTo == next);
}

TEST_CASE("Group output values undo/redo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));

    auto &group = f.engine().getPatch()->getPart(0)->getGroup(0);

    SECTION("bool (muted)")
    {
        REQUIRE(!group->outputInfo.muted);
        f.send(cmsg::UpdateGroupOutputBoolValue(
            {offsetof(scxt::engine::Group::GroupOutputInfo, muted), true}));
        REQUIRE(group->outputInfo.muted);

        f.sendUndo();
        REQUIRE(!group->outputInfo.muted);

        f.sendRedo();
        REQUIRE(group->outputInfo.muted);
    }

    SECTION("int16 (pitch bend up)")
    {
        auto orig = group->outputInfo.pbUp;
        f.send(cmsg::UpdateGroupOutputInt16TValue(
            {offsetof(scxt::engine::Group::GroupOutputInfo, pbUp), (int16_t)(orig + 5)}));
        REQUIRE(group->outputInfo.pbUp == orig + 5);

        f.sendUndo();
        REQUIRE(group->outputInfo.pbUp == orig);

        f.sendRedo();
        REQUIRE(group->outputInfo.pbUp == orig + 5);
    }
}

TEST_CASE("Mute solo group undo/redo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));

    auto &group = f.engine().getPatch()->getPart(0)->getGroup(0);
    REQUIRE(!group->outputInfo.muted);

    f.send(cmsg::MuteOrSoloGroup({0, 0, true, false, false}));
    REQUIRE(group->outputInfo.muted);

    f.sendUndo();
    REQUIRE(!group->outputInfo.muted);

    f.sendRedo();
    REQUIRE(group->outputInfo.muted);
}

TEST_CASE("EG bool undo/redo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    REQUIRE(!zone->egStorage[0].isTemposync);

    f.send(cmsg::UpdateZoneOrGroupEGBoolValue(
        {true, 0, offsetof(scxt::modulation::modulators::AdsrStorage, isTemposync), true}));
    REQUIRE(zone->egStorage[0].isTemposync);

    f.sendUndo();
    REQUIRE(!zone->egStorage[0].isTemposync);

    f.sendRedo();
    REQUIRE(zone->egStorage[0].isTemposync);
}

TEST_CASE("Full ADSR storage undo/redo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto orig = zone->egStorage[1];

    auto next = orig;
    next.a = 0.7f;
    next.r = 0.2f;
    f.send(cmsg::UpdateFullAdsrStorageForGroupsOrZones({true, 1, next}));
    REQUIRE(zone->egStorage[1].a == Approx(0.7f));

    f.sendUndo();
    REQUIRE(zone->egStorage[1].a == Approx(orig.a));
    REQUIRE(zone->egStorage[1].r == Approx(orig.r));

    f.sendRedo();
    REQUIRE(zone->egStorage[1].a == Approx(0.7f));
    REQUIRE(zone->egStorage[1].r == Approx(0.2f));
}

TEST_CASE("Modulator shape change undo/redo multiselect", "[undo]")
{
    using MS = scxt::modulation::ModulatorStorage;

    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}, {0, 0, 1, true, false, false}}));

    auto &z0 = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto &z1 = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(1);
    REQUIRE(z0->modulatorStorage[0].modulatorShape == MS::LFO_SINE);
    REQUIRE(z1->modulatorStorage[0].modulatorShape == MS::LFO_SINE);

    f.send(cmsg::UpdateZoneOrGroupModStorageInt16TValue(
        {true, 0, offsetof(MS, modulatorShape), (int16_t)MS::LFO_TRI}));
    REQUIRE(z0->modulatorStorage[0].modulatorShape == MS::LFO_TRI);
    REQUIRE(z1->modulatorStorage[0].modulatorShape == MS::LFO_TRI);

    f.sendUndo();
    REQUIRE(z0->modulatorStorage[0].modulatorShape == MS::LFO_SINE);
    REQUIRE(z1->modulatorStorage[0].modulatorShape == MS::LFO_SINE);

    f.sendRedo();
    REQUIRE(z0->modulatorStorage[0].modulatorShape == MS::LFO_TRI);
    REQUIRE(z1->modulatorStorage[0].modulatorShape == MS::LFO_TRI);
}

TEST_CASE("Full modstorage undo/redo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto orig = zone->modulatorStorage[0];

    auto next = orig;
    next.rate = 2.5f;
    f.send(cmsg::UpdateFullModStorageForGroupsOrZones({true, 0, next}));
    REQUIRE(zone->modulatorStorage[0].rate == Approx(2.5f));

    f.sendUndo();
    REQUIRE(zone->modulatorStorage[0].rate == Approx(orig.rate));

    f.sendRedo();
    REQUIRE(zone->modulatorStorage[0].rate == Approx(2.5f));
}

TEST_CASE("Delete variant undo restores sample", "[undo]")
{
    namespace fs = std::filesystem;
    auto sampleP =
        fs::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / "next" / "Beep.wav";
    REQUIRE(fs::exists(sampleP));

    UndoFixture f;
    f.send(cmsg::AddSampleWithRange({sampleP.u8string(), 60, 48, 72, 0, 127}), 30);
    auto &part = f.engine().getPatch()->getPart(0);
    REQUIRE(part->getGroups().size() == 1);
    auto &zone = part->getGroup(0)->getZone(0);
    REQUIRE(zone->variantData.variants[0].active);
    REQUIRE(zone->samplePointers[0]);

    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
    f.send(cmsg::DeleteVariant(0), 30);
    REQUIRE(!zone->variantData.variants[0].active);

    f.sendUndo(30);
    REQUIRE(zone->variantData.variants[0].active);
    REQUIRE(zone->samplePointers[0]);

    f.sendRedo(30);
    REQUIRE(!zone->variantData.variants[0].active);
}

TEST_CASE("Processor float gesture coalesces to one undo", "[undo]")
{
    using scxt::dsp::processor::proct_CytomicSVF;

    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
    f.send(cmsg::SetSelectedProcessorType({true, (int32_t)0, (int32_t)proct_CytomicSVF}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto origMix = zone->processorStorage[0].mix;
    auto baseSize = f.undoManager().undoStackSize();

    f.send(cmsg::BeginEdit({(int32_t)cmsg::EditSubtree::processor, true, 0}));
    REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

    for (int i = 0; i < 15; ++i)
    {
        f.send(cmsg::UpdateZoneOrGroupProcessorFloatValue(
            {true, 0, offsetof(scxt::dsp::processor::ProcessorStorage, mix), 0.3f + 0.01f * i}));
    }
    REQUIRE(zone->processorStorage[0].mix == Approx(0.44f));
    REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

    f.sendUndo();
    REQUIRE(zone->processorStorage[0].mix == Approx(origMix));

    f.sendRedo();
    REQUIRE(zone->processorStorage[0].mix == Approx(0.44f));
}

TEST_CASE("Swap zone processors undo/redo", "[undo]")
{
    using scxt::dsp::processor::proct_CytomicSVF;
    using scxt::dsp::processor::proct_none;

    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
    f.send(cmsg::SetSelectedProcessorType({true, (int32_t)0, (int32_t)proct_CytomicSVF}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    REQUIRE(zone->processorStorage[0].type == proct_CytomicSVF);
    REQUIRE(zone->processorStorage[1].type == proct_none);

    // payload is {to, from, copy}
    f.send(cmsg::SwapZoneProcessors({1, 0, false}));
    REQUIRE(zone->processorStorage[0].type == proct_none);
    REQUIRE(zone->processorStorage[1].type == proct_CytomicSVF);

    f.sendUndo();
    REQUIRE(zone->processorStorage[0].type == proct_CytomicSVF);
    REQUIRE(zone->processorStorage[1].type == proct_none);

    f.sendRedo();
    REQUIRE(zone->processorStorage[0].type == proct_none);
    REQUIRE(zone->processorStorage[1].type == proct_CytomicSVF);
}

TEST_CASE("Full processor storage undo/redo", "[undo]")
{
    using scxt::dsp::processor::proct_CytomicSVF;
    using scxt::dsp::processor::proct_none;

    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);

    auto ps = zone->processorStorage[0];
    ps.type = proct_CytomicSVF;
    ps.mix = 0.66f;
    f.send(cmsg::SendFullProcessorStorage({true, 0, ps}));
    REQUIRE(zone->processorStorage[0].type == proct_CytomicSVF);
    REQUIRE(zone->processorStorage[0].mix == Approx(0.66f));

    f.sendUndo();
    REQUIRE(zone->processorStorage[0].type == proct_none);

    f.sendRedo();
    REQUIRE(zone->processorStorage[0].type == proct_CytomicSVF);
    REQUIRE(zone->processorStorage[0].mix == Approx(0.66f));
}

TEST_CASE("Mod row drag gesture coalesces to one undo", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));

    auto &zone = f.engine().getPatch()->getPart(0)->getGroup(0)->getZone(0);
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.f));
    auto baseSize = f.undoManager().undoStackSize();

    f.send(cmsg::BeginEdit({(int32_t)cmsg::EditSubtree::mod_row, true, 0}));
    REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

    for (int i = 0; i < 15; ++i)
    {
        auto row = zone->routingTable.routes[0];
        row.depth = 0.1f + 0.01f * i;
        f.send(cmsg::UpdateZoneRoutingRow({0, row, false}));
    }
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.24f));
    REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

    f.send(cmsg::EndEdit(true));

    f.sendUndo();
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.f));

    f.sendRedo();
    REQUIRE(zone->routingTable.routes[0].depth == Approx(0.24f));
}

TEST_CASE("Add blank zone undo deletes zone", "[undo]")
{
    UndoFixture f;
    auto &part = f.engine().getPatch()->getPart(0);

    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    REQUIRE(part->getGroups().size() == 1);
    REQUIRE(part->getGroup(0)->getZones().size() == 1);

    f.send(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    REQUIRE(part->getGroup(0)->getZones().size() == 2);

    f.sendUndo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 1);

    f.sendRedo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 2);
    REQUIRE(part->getGroup(0)->getZone(1)->mapping.keyboardRange.keyStart == 61);
}

TEST_CASE("Delete zone undo resurrects", "[undo]")
{
    namespace fs = std::filesystem;
    auto sampleP =
        fs::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / "next" / "Beep.wav";
    REQUIRE(fs::exists(sampleP));

    UndoFixture f;
    f.send(cmsg::AddSampleWithRange({sampleP.u8string(), 60, 48, 72, 0, 127}), 30);
    auto &part = f.engine().getPatch()->getPart(0);
    REQUIRE(part->getGroup(0)->getZones().size() == 1);
    auto zoneName = part->getGroup(0)->getZone(0)->getName();

    f.send(cmsg::DeleteZone(ZoneAddress{0, 0, 0}), 30);
    REQUIRE(part->getGroup(0)->getZones().size() == 0);

    f.sendUndo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 1);
    auto &rz = part->getGroup(0)->getZone(0);
    REQUIRE(rz->getName() == zoneName);
    REQUIRE(rz->variantData.variants[0].active);
    REQUIRE(rz->samplePointers[0]);

    // resurrected zone becomes the lead selection
    auto lz = f.engine().getSelectionManager()->currentLeadZone(f.engine());
    REQUIRE(lz.has_value());
    REQUIRE(lz->zone == 0);

    f.sendRedo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 0);
}

TEST_CASE("Delete selected zones undo resurrects all", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 10, 20, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 21, 30, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 31, 40, 0, 127}));
    auto &part = f.engine().getPatch()->getPart(0);
    REQUIRE(part->getGroup(0)->getZones().size() == 3);

    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true},
                                     {0, 0, 1, true, false, false},
                                     {0, 0, 2, true, false, false}}));
    f.send(cmsg::DeleteAllSelectedZones(true), 30);
    REQUIRE(part->getGroup(0)->getZones().size() == 0);

    f.sendUndo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 3);
    REQUIRE(part->getGroup(0)->getZone(0)->mapping.keyboardRange.keyStart == 10);
    REQUIRE(part->getGroup(0)->getZone(1)->mapping.keyboardRange.keyStart == 21);
    REQUIRE(part->getGroup(0)->getZone(2)->mapping.keyboardRange.keyStart == 31);

    f.sendRedo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 0);
}

TEST_CASE("Duplicate zone undo removes copy", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    auto &part = f.engine().getPatch()->getPart(0);

    f.send(cmsg::DuplicateZone(ZoneAddress{0, 0, 0}), 30);
    REQUIRE(part->getGroup(0)->getZones().size() == 2);

    f.sendUndo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 1);

    f.sendRedo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 2);
}

TEST_CASE("Paste zone undo removes pasted", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    auto &part = f.engine().getPatch()->getPart(0);

    f.send(cmsg::CopyZone(ZoneAddress{0, 0, 0}));
    f.send(cmsg::PasteZone(ZoneAddress{0, 0, 0}), 30);
    REQUIRE(part->getGroup(0)->getZones().size() == 2);

    f.sendUndo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 1);

    f.sendRedo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 2);
}

TEST_CASE("Move zone undo returns to source group", "[undo]")
{
    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::CreateGroup(0), 20);
    auto &part = f.engine().getPatch()->getPart(0);
    REQUIRE(part->getGroups().size() == 2);

    f.send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
    f.send(cmsg::MoveZonesFromTo({{ZoneAddress{0, 0, 0}}, ZoneAddress{0, 1, -1}}), 30);
    REQUIRE(part->getGroup(0)->getZones().size() == 0);
    REQUIRE(part->getGroup(1)->getZones().size() == 1);

    f.sendUndo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 1);
    REQUIRE(part->getGroup(1)->getZones().size() == 0);

    f.sendRedo(30);
    REQUIRE(part->getGroup(0)->getZones().size() == 0);
    REQUIRE(part->getGroup(1)->getZones().size() == 1);
}

TEST_CASE("Create group undo deletes group", "[undo]")
{
    UndoFixture f;
    auto &part = f.engine().getPatch()->getPart(0);
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    REQUIRE(part->getGroups().size() == 1);

    f.send(cmsg::CreateGroup(0), 20);
    REQUIRE(part->getGroups().size() == 2);

    f.sendUndo(30);
    REQUIRE(part->getGroups().size() == 1);

    f.sendRedo(30);
    REQUIRE(part->getGroups().size() == 2);
}

TEST_CASE("Delete group undo resurrects with zones", "[undo]")
{
    UndoFixture f;
    auto &part = f.engine().getPatch()->getPart(0);
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"Keepers"}}));
    REQUIRE(part->getGroup(0)->getZones().size() == 2);

    f.send(cmsg::DeleteGroup(ZoneAddress{0, 0, -1}), 30);
    REQUIRE(part->getGroups().size() == 0);

    f.sendUndo(40);
    REQUIRE(part->getGroups().size() == 1);
    REQUIRE(part->getGroup(0)->name == "Keepers");
    REQUIRE(part->getGroup(0)->getZones().size() == 2);

    f.sendRedo(40);
    REQUIRE(part->getGroups().size() == 0);
}

TEST_CASE("Clear part undo restores groups", "[undo]")
{
    UndoFixture f;
    auto &part = f.engine().getPatch()->getPart(0);
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::CreateGroup(0), 20);
    f.send(cmsg::AddBlankZone({0, 1, 61, 72, 0, 127}));
    REQUIRE(part->getGroups().size() == 2);

    f.send(cmsg::ClearPart(0), 30);
    REQUIRE(part->getGroups().size() == 0);

    f.sendUndo(40);
    REQUIRE(part->getGroups().size() == 2);
    REQUIRE(part->getGroup(0)->getZones().size() == 1);
    REQUIRE(part->getGroup(1)->getZones().size() == 1);

    f.sendRedo(40);
    REQUIRE(part->getGroups().size() == 0);
}

TEST_CASE("Move group undo returns to source position", "[undo]")
{
    UndoFixture f;
    auto &part = f.engine().getPatch()->getPart(0);
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::CreateGroup(0), 20);
    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"GroupA"}}));
    f.send(cmsg::RenameGroup({ZoneAddress{0, 1, -1}, std::string{"GroupB"}}));

    // swap form (second address zone < 0)
    f.send(cmsg::MoveGroupTo({ZoneAddress{0, 0, -1}, ZoneAddress{0, 1, -1}}), 30);
    REQUIRE(part->getGroup(0)->name == "GroupB");
    REQUIRE(part->getGroup(1)->name == "GroupA");

    f.sendUndo(30);
    REQUIRE(part->getGroup(0)->name == "GroupA");
    REQUIRE(part->getGroup(1)->name == "GroupB");

    f.sendRedo(30);
    REQUIRE(part->getGroup(0)->name == "GroupB");
    REQUIRE(part->getGroup(1)->name == "GroupA");
}

TEST_CASE("Duplicate group undo removes copy", "[undo]")
{
    UndoFixture f;
    auto &part = f.engine().getPatch()->getPart(0);
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));

    f.send(cmsg::DuplicateGroup(ZoneAddress{0, 0, -1}), 30);
    REQUIRE(part->getGroups().size() == 2);
    REQUIRE(part->getGroup(1)->getZones().size() == 1);

    f.sendUndo(30);
    REQUIRE(part->getGroups().size() == 1);

    f.sendRedo(30);
    REQUIRE(part->getGroups().size() == 2);
}

TEST_CASE("Activate and deactivate part undo/redo", "[undo]")
{
    UndoFixture f;
    auto &patch = f.engine().getPatch();
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    REQUIRE(patch->getPart(0)->configuration.active);
    REQUIRE(!patch->getPart(1)->configuration.active);

    f.send(cmsg::ActivateNextPart(true), 30);
    REQUIRE(patch->getPart(1)->configuration.active);

    f.sendUndo(40);
    REQUIRE(!patch->getPart(1)->configuration.active);

    f.sendRedo(40);
    REQUIRE(patch->getPart(1)->configuration.active);

    f.send(cmsg::DeactivatePart(1), 30);
    REQUIRE(!patch->getPart(1)->configuration.active);

    f.sendUndo(40);
    REQUIRE(patch->getPart(1)->configuration.active);
}

TEST_CASE("Mixer bus effect undo/redo", "[undo]")
{
    UndoFixture f;
    auto &bus = f.engine().getPatch()->busses.busByAddress(scxt::engine::BusAddress::AUX_0);
    REQUIRE(bus.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::none);

    // bus AUX_0, no part, slot 0 → reverb1
    f.send(cmsg::SetBusEffectToType({(int)scxt::engine::BusAddress::AUX_0, -1, 0,
                                     (int)scxt::engine::AvailableBusEffects::reverb1}),
           30);
    REQUIRE(bus.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::reverb1);

    f.sendUndo(30);
    REQUIRE(bus.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::none);

    f.sendRedo(30);
    REQUIRE(bus.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::reverb1);
}

TEST_CASE("Mixer bus send undo/redo", "[undo]")
{
    UndoFixture f;
    auto &bus = f.engine().getPatch()->busses.busByAddress(scxt::engine::BusAddress::PART_0);
    auto orig = bus.busSendStorage;

    auto next = orig;
    next.sendLevels[0] = 0.8f;
    f.send(cmsg::SetBusSendStorage({(int)scxt::engine::BusAddress::PART_0, next}), 20);
    REQUIRE(bus.busSendStorage.sendLevels[0] == Approx(0.8f));

    f.sendUndo(20);
    REQUIRE(bus.busSendStorage.sendLevels[0] == Approx(orig.sendLevels[0]));

    f.sendRedo(20);
    REQUIRE(bus.busSendStorage.sendLevels[0] == Approx(0.8f));
}

TEST_CASE("Mixer bus effect gesture coalesces to one undo", "[undo]")
{
    UndoFixture f;
    auto busAddr = (int)scxt::engine::BusAddress::AUX_0;
    auto &bus = f.engine().getPatch()->busses.busByAddress(scxt::engine::BusAddress::AUX_0);

    f.send(
        cmsg::SetBusEffectToType({busAddr, -1, 0, (int)scxt::engine::AvailableBusEffects::reverb1}),
        30);
    auto origP0 = bus.busEffectStorage[0].params[0];
    auto baseSize = f.undoManager().undoStackSize();

    f.send(cmsg::BeginEdit({(int32_t)cmsg::EditSubtree::bus_effect, false,
                            scxt::undo::mixerUndoIndex(busAddr, -1, 0)}));
    REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

    for (int i = 0; i < 12; ++i)
    {
        auto bes = bus.busEffectStorage[0];
        bes.params[0] = 0.1f + 0.01f * i;
        f.send(cmsg::SetBusEffectStorage({busAddr, -1, 0, bes}));
    }
    REQUIRE(bus.busEffectStorage[0].params[0] == Approx(0.21f));
    REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

    f.send(cmsg::EndEdit(true));

    f.sendUndo(20);
    REQUIRE(bus.busEffectStorage[0].params[0] == Approx(origP0));

    f.sendRedo(20);
    REQUIRE(bus.busEffectStorage[0].params[0] == Approx(0.21f));
}

TEST_CASE("Mixer bus send gesture coalesces to one undo", "[undo]")
{
    UndoFixture f;
    auto busAddr = (int)scxt::engine::BusAddress::PART_0;
    auto &bus = f.engine().getPatch()->busses.busByAddress(scxt::engine::BusAddress::PART_0);
    auto orig = bus.busSendStorage;
    auto baseSize = f.undoManager().undoStackSize();

    // the begin payload carries the raw bus; the handler derives the tag
    f.send(cmsg::BeginEdit({(int32_t)cmsg::EditSubtree::bus_send, false, busAddr}));
    REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

    for (int i = 0; i < 12; ++i)
    {
        auto bss = bus.busSendStorage;
        bss.sendLevels[0] = 0.2f + 0.01f * i;
        f.send(cmsg::SetBusSendStorage({busAddr, bss}));
    }
    REQUIRE(bus.busSendStorage.sendLevels[0] == Approx(0.31f));
    REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

    f.send(cmsg::EndEdit(true));

    f.sendUndo(20);
    REQUIRE(bus.busSendStorage.sendLevels[0] == Approx(orig.sendLevels[0]));

    f.sendRedo(20);
    REQUIRE(bus.busSendStorage.sendLevels[0] == Approx(0.31f));
}

TEST_CASE("Part config undo/redo", "[undo]")
{
    UndoFixture f;
    auto &part = f.engine().getPatch()->getPart(0);
    auto orig = part->configuration;

    auto next = orig;
    next.channel = 5;
    f.send(cmsg::UpdatePartFullConfig({(int16_t)0, next}), 20);
    REQUIRE(part->configuration.channel == 5);

    f.sendUndo(20);
    REQUIRE(part->configuration.channel == orig.channel);

    f.sendRedo(20);
    REQUIRE(part->configuration.channel == 5);
}

TEST_CASE("Swap bus fx undo/redo", "[undo]")
{
    UndoFixture f;
    auto &b1 = f.engine().getPatch()->busses.busByAddress(scxt::engine::BusAddress::AUX_0);
    auto aux1 = (scxt::engine::BusAddress)((int)scxt::engine::BusAddress::AUX_0 + 1);
    auto &b2 = f.engine().getPatch()->busses.busByAddress(aux1);

    f.send(cmsg::SetBusEffectToType({(int)scxt::engine::BusAddress::AUX_0, -1, 0,
                                     (int)scxt::engine::AvailableBusEffects::reverb1}),
           30);
    REQUIRE(b1.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::reverb1);
    REQUIRE(b2.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::none);

    f.send(cmsg::SwapBusFX({(int16_t)scxt::engine::BusAddress::AUX_0, 0, (int16_t)aux1, 0, 0}), 30);
    REQUIRE(b1.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::none);
    REQUIRE(b2.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::reverb1);

    f.sendUndo(30);
    REQUIRE(b1.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::reverb1);
    REQUIRE(b2.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::none);

    f.sendRedo(30);
    REQUIRE(b1.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::none);
    REQUIRE(b2.busEffectStorage[0].type == scxt::engine::AvailableBusEffects::reverb1);
}

TEST_CASE("Stack clearing messages clear undo and redo", "[undo]")
{
    namespace fs = std::filesystem;

    UndoFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));

    // Build a non-empty undo and redo stack
    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"FirstName"}}));
    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"SecondName"}}));
    f.sendUndo();
    REQUIRE(f.undoManager().hasUndoSteps());
    REQUIRE(f.undoManager().hasRedoSteps());

    SECTION("reset engine clears")
    {
        f.send(cmsg::ResetEngine(std::string{"InitSampler.dat"}), 30);
    }

    SECTION("unstream engine state clears")
    {
        auto state = scxt::json::streamEngineState(f.engine());
        f.send(cmsg::UnstreamEngineState(state), 30);
    }

    REQUIRE(!f.undoManager().hasUndoSteps());
    REQUIRE(!f.undoManager().hasRedoSteps());
}

TEST_CASE("Replace part with instrument is one undo entry", "[undo]")
{
    namespace fs = std::filesystem;
    auto sf2P = fs::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / "harpsi.sf2";
    REQUIRE(fs::exists(sf2P));

    UndoFixture f;
    auto &part = f.engine().getPatch()->getPart(0);

    // a complex-ish prior state
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"ComplexPatch"}}));
    REQUIRE(part->getGroup(0)->getZones().size() == 2);
    auto baseSize = f.undoManager().undoStackSize();

    // the message sequence the mapping pane sends for drop-to-replace
    f.send(cmsg::BeginEdit({(int32_t)cmsg::EditSubtree::part_stream, false, 0}));
    f.send(cmsg::ClearPart(0), 30);
    f.send(cmsg::AddSample(sf2P.u8string()), 60);

    bool importChangedPart =
        part->getGroups().size() != 1 || part->getGroup(0)->name != "ComplexPatch";
    REQUIRE(importChangedPart);
    REQUIRE(f.undoManager().undoStackSize() == baseSize + 1);

    // one undo restores the prior patch, not the cleared blank state
    f.sendUndo(60);
    REQUIRE(part->getGroups().size() == 1);
    REQUIRE(part->getGroup(0)->name == "ComplexPatch");
    REQUIRE(part->getGroup(0)->getZones().size() == 2);

    f.sendRedo(60);
    bool redoChangedPart =
        part->getGroups().size() != 1 || part->getGroup(0)->name != "ComplexPatch";
    REQUIRE(redoChangedPart);
}

TEST_CASE("Load part into undo restores prior part", "[undo]")
{
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "scxt-undo-load-part";
    fs::create_directories(dir);
    auto pp = dir / "preload.scp";

    UndoFixture f;
    auto &part = f.engine().getPatch()->getPart(0);

    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"SavedName"}}));
    f.send(cmsg::SavePart({pp.u8string(), 0, (int)scxt::patch_io::SaveStyles::NO_SAMPLES}), 30);
    REQUIRE(fs::exists(pp));

    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"PreLoadName"}}));
    f.send(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    REQUIRE(part->getGroup(0)->getZones().size() == 2);

    f.send(cmsg::LoadPartInto({pp.u8string(), (int16_t)0}), 40);
    REQUIRE(part->getGroup(0)->name == "SavedName");
    REQUIRE(part->getGroup(0)->getZones().size() == 1);

    f.sendUndo(40);
    REQUIRE(part->getGroup(0)->name == "PreLoadName");
    REQUIRE(part->getGroup(0)->getZones().size() == 2);

    f.sendRedo(40);
    REQUIRE(part->getGroup(0)->name == "SavedName");
    REQUIRE(part->getGroup(0)->getZones().size() == 1);

    fs::remove_all(dir);
}

TEST_CASE("Load multi undo restores prior engine", "[undo]")
{
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "scxt-undo-load-multi";
    fs::create_directories(dir);
    auto mp = dir / "preload.scm";

    UndoFixture f;
    auto &part = f.engine().getPatch()->getPart(0);

    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"SavedName"}}));
    f.send(cmsg::SaveMulti({mp.u8string(), (int)scxt::patch_io::SaveStyles::NO_SAMPLES}), 30);
    REQUIRE(fs::exists(mp));

    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"PreLoadName"}}));
    f.send(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    REQUIRE(part->getGroup(0)->getZones().size() == 2);

    f.send(cmsg::LoadMulti(mp.u8string()), 40);
    REQUIRE(part->getGroup(0)->name == "SavedName");
    REQUIRE(part->getGroup(0)->getZones().size() == 1);

    f.sendUndo(40);
    REQUIRE(part->getGroup(0)->name == "PreLoadName");
    REQUIRE(part->getGroup(0)->getZones().size() == 2);

    f.sendRedo(40);
    REQUIRE(part->getGroup(0)->name == "SavedName");
    REQUIRE(part->getGroup(0)->getZones().size() == 1);

    fs::remove_all(dir);
}

TEST_CASE("SF2 import undo restores prior part", "[undo]")
{
    namespace fs = std::filesystem;
    auto sf2P = fs::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / "harpsi.sf2";
    REQUIRE(fs::exists(sf2P));

    UndoFixture f;
    auto &part = f.engine().getPatch()->getPart(0);

    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"BeforeImport"}}));
    REQUIRE(part->getGroups().size() == 1);

    f.send(cmsg::AddSample(sf2P.u8string()), 60);
    bool importChangedPart =
        part->getGroups().size() != 1 || part->getGroup(0)->name != "BeforeImport";
    REQUIRE(importChangedPart);

    f.sendUndo(60);
    REQUIRE(part->getGroups().size() == 1);
    REQUIRE(part->getGroup(0)->name == "BeforeImport");
    REQUIRE(part->getGroup(0)->getZones().size() == 1);
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
