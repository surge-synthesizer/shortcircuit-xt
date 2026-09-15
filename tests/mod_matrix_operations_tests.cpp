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
#include "modulation/voice_matrix.h"
#include "modulation/group_matrix.h"
#include "test_utils.h"

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

namespace mod_target_proc_change_test
{
// the label a mod row shows for its target, and whether it paints enabled
template <typename MD, typename TI> std::pair<std::string, bool> rowLabel(const MD &md, const TI &t)
{
    for (const auto &[ti, dn, ca, en, sep] : std::get<2>(md))
        if (ti == t)
            return {std::get<2>(dn) + ": " + std::get<3>(dn), en};
    return {"", false};
}

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
    void setType(scxt::dsp::processor::ProcessorType t)
    {
        auto bg = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        zone().setProcessorType(0, t);
        group().setProcessorType(0, t);
    }
};
} // namespace mod_target_proc_change_test

TEST_CASE("Mod targets on an emptied processor slot still label themselves", "[modulation]")
{
    namespace mt = mod_target_proc_change_test;
    using scxt::dsp::processor::proct_CytomicSVF;
    using scxt::dsp::processor::proct_none;

    mt::Fixture f;
    auto zt = scxt::voice::modulation::MatrixEndpoints::ProcessorTarget::floatParam(0, 0);
    auto gt = scxt::modulation::GroupMatrixEndpoints::ProcessorTarget::floatParam(0, 0);

    f.setType(proct_CytomicSVF);
    auto [zl, zen] = mt::rowLabel(scxt::voice::modulation::getVoiceMatrixMetadata(f.zone()), zt);
    auto [gl, gen] = mt::rowLabel(scxt::modulation::getGroupMatrixMetadata(f.group()), gt);
    REQUIRE(zl.rfind("P1.", 0) == 0);
    REQUIRE(zen);
    REQUIRE(gl.rfind("P1.", 0) == 0);
    REQUIRE(gen);

    f.setType(proct_none);
    std::tie(zl, zen) = mt::rowLabel(scxt::voice::modulation::getVoiceMatrixMetadata(f.zone()), zt);
    std::tie(gl, gen) = mt::rowLabel(scxt::modulation::getGroupMatrixMetadata(f.group()), gt);
    REQUIRE(zl == "P1: -");
    REQUIRE(!zen);
    REQUIRE(gl == "P1: -");
    REQUIRE(!gen);
}

TEST_CASE("Mod targets past a smaller processor's parameters still label themselves",
          "[modulation]")
{
    namespace mt = mod_target_proc_change_test;
    namespace proc = scxt::dsp::processor;

    mt::Fixture f;
    f.setType(proc::proct_CytomicSVF);
    auto bigCount = f.zone().processorDescription[0].numFloatParams;
    REQUIRE(bigCount > 1);

    // any processor with fewer float params will do
    auto smaller = proc::proct_none;
    for (int t = proc::proct_none + 1; t < proc::proct_num_types; ++t)
    {
        if (!proc::isProcessorImplemented((proc::ProcessorType)t))
            continue;
        f.setType((proc::ProcessorType)t);
        auto n = f.zone().processorDescription[0].numFloatParams;
        if (n > 0 && n < bigCount)
        {
            smaller = (proc::ProcessorType)t;
            break;
        }
    }
    REQUIRE(smaller != proc::proct_none);
    f.setType(smaller);

    auto zt =
        scxt::voice::modulation::MatrixEndpoints::ProcessorTarget::floatParam(0, bigCount - 1);
    auto gt = scxt::modulation::GroupMatrixEndpoints::ProcessorTarget::floatParam(0, bigCount - 1);
    auto [zl, zen] = mt::rowLabel(scxt::voice::modulation::getVoiceMatrixMetadata(f.zone()), zt);
    auto [gl, gen] = mt::rowLabel(scxt::modulation::getGroupMatrixMetadata(f.group()), gt);

    INFO("zone label '" << zl << "' group label '" << gl << "'");
    REQUIRE(zl.size() > 3);
    REQUIRE(zl.substr(zl.size() - 3) == ": -");
    REQUIRE(!zen);
    REQUIRE(gl.substr(gl.size() - 3) == ": -");
    REQUIRE(!gen);

    // and a parameter the processor does have is untouched
    auto zt0 = scxt::voice::modulation::MatrixEndpoints::ProcessorTarget::floatParam(0, 0);
    auto [zl0, zen0] = mt::rowLabel(scxt::voice::modulation::getVoiceMatrixMetadata(f.zone()), zt0);
    REQUIRE(zl0.substr(zl0.size() - 3) != ": -");
}

TEST_CASE("A multiplicative route survives its processor switching to none", "[modulation]")
{
    namespace mt = mod_target_proc_change_test;
    namespace proc = scxt::dsp::processor;
    using TI = scxt::modulation::shared::TargetIdentifier;
    using SI = scxt::modulation::shared::SourceIdentifier;

    mt::Fixture f;

    // any float param that takes multiplicative modulation will do
    int paramIdx{-1};
    for (int t = proc::proct_none + 1; t < proc::proct_num_types && paramIdx < 0; ++t)
    {
        if (!proc::isProcessorImplemented((proc::ProcessorType)t))
            continue;
        f.setType((proc::ProcessorType)t);
        const auto &d = f.zone().processorDescription[0];
        for (int i = 0; i < d.numFloatParams; ++i)
        {
            if (d.floatControlDescriptions[i].hasSupportsMultiplicativeModulation())
            {
                paramIdx = i;
                break;
            }
        }
    }
    REQUIRE(paramIdx >= 0);

    auto &row = f.zone().routingTable.routes[0];
    row.source = SI{'zmac', 'mcro', 0};
    row.target = scxt::voice::modulation::MatrixEndpoints::ProcessorTarget::floatParam(0, paramIdx);
    row.depth = 0.5f;
    row.applicationMode = sst::basic_blocks::mod_matrix::ApplicationMode::MULTIPLICATIVE;

    f.setType(proc::proct_none);
    REQUIRE(f.zone().routingTable.routes[0].applicationMode ==
            sst::basic_blocks::mod_matrix::ApplicationMode::ADDITIVE);

    // binding the voice matrix used to assert on a multiplicative route to an additive target
    f.eng->processNoteOnEvent(0, 0, 60, -1, 1.f, 0.f);
    for (int i = 0; i < 4; ++i)
        f.eng->processAudio();
    f.eng->processNoteOffEvent(0, 0, 60, -1, 0.f);
    for (int i = 0; i < 4; ++i)
        f.eng->processAudio();
}

TEST_CASE("Processors name only the float params they count", "[modulation]")
{
    // mod targets past the count read as absent, so a named one out there would vanish
    namespace mt = mod_target_proc_change_test;
    namespace proc = scxt::dsp::processor;

    mt::Fixture f;
    for (int t = proc::proct_none + 1; t < proc::proct_num_types; ++t)
    {
        if (!proc::isProcessorImplemented((proc::ProcessorType)t))
            continue;
        f.setType((proc::ProcessorType)t);
        const auto &d = f.zone().processorDescription[0];
        for (int i = d.numFloatParams; i < scxt::maxProcessorFloatParams; ++i)
        {
            INFO(d.typeDisplayName << " param " << i);
            REQUIRE(d.floatControlDescriptions[i].name.empty());
        }
    }
}
