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

/*
 * A client can send anything. These check that payload indices which are out of range
 * are refused on the serialization thread rather than indexing the engine out of bounds.
 */

#include "catch2/catch2.hpp"
#include "engine/engine.h"
#include "console_harness.h"

namespace cmsg = scxt::messaging::client;
using ZoneAddress = scxt::selection::SelectionManager::ZoneAddress;

TEST_CASE("Out of range macro messages are refused")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    auto &part = th.engine->getPatch()->getPart(0);
    auto before = part->macros[0].value;

    th.sendToSerialization(cmsg::SetMacroValue({scxt::numParts, 0, 0.75f}));
    th.sendToSerialization(cmsg::SetMacroValue({-1, 0, 0.75f}));
    th.sendToSerialization(cmsg::SetMacroValue({0, (int16_t)scxt::macrosPerPart, 0.75f}));
    th.sendToSerialization(cmsg::SetMacroValue({0, -1, 0.75f}));
    th.stepUI();

    REQUIRE(part->macros[0].value == before);

    th.sendToSerialization(cmsg::MacroBeginEndEdit({true, scxt::numParts, 0}));
    th.sendToSerialization(cmsg::MacroBeginEndEdit({true, 0, (int16_t)scxt::macrosPerPart}));
    th.sendToSerialization(cmsg::MacroBeginEndEdit({false, -1, -1}));
    th.stepUI();

    // and an in-range one still lands
    th.sendToSerialization(cmsg::SetMacroValue({0, 0, 0.75f}));
    th.stepUI();
    REQUIRE(part->macros[0].value == 0.75f);
}

TEST_CASE("Out of range mod row reorder is refused")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.stepUI();

    auto &zone = th.engine->getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto &routes = zone->routingTable.routes;
    routes[0].active = false;
    routes[1].active = true;

    const auto n = (int)scxt::modMatrixRowsPerZone;
    for (auto bad : {n, n + 40, -1})
    {
        th.sendToSerialization(cmsg::ReorderModRow({true, bad, 0, false}));
        th.sendToSerialization(cmsg::ReorderModRow({true, 0, bad, true}));
        th.sendToSerialization(cmsg::ReorderModRow({false, bad, 0, false}));
        th.sendToSerialization(cmsg::ReorderModRow({false, 0, bad, true}));
    }
    th.stepUI();

    REQUIRE_FALSE(routes[0].active);
    REQUIRE(routes[1].active);

    // an in-range swap still works
    th.sendToSerialization(cmsg::ReorderModRow({true, 0, 1, false}));
    th.stepUI();
    REQUIRE(routes[0].active);
    REQUIRE_FALSE(routes[1].active);
}

TEST_CASE("An out of range variant field offset is refused")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.stepUI();

    auto &zone = th.engine->getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto before = zone->variantData.variants[0];

    scxt::engine::Zone::SingleVariant edited;
    edited.startSample = 12345;

    const auto vsz = sizeof(scxt::engine::Zone::SingleVariant);
    th.sendToSerialization(cmsg::UpdateVariantField({0, false, (ptrdiff_t)vsz, 8, edited}));
    th.sendToSerialization(cmsg::UpdateVariantField({0, false, -8, 8, edited}));
    th.sendToSerialization(cmsg::UpdateVariantField({0, false, 0, vsz + 8, edited}));
    th.sendToSerialization(cmsg::UpdateVariantField({0, true, 0, (size_t)-1, edited}));
    th.stepUI();

    REQUIRE(zone->variantData.variants[0].startSample == before.startSample);

    // the control: the same field named legally does copy. Direct rather than through a
    // message, since a blank zone has no sample for a frame position to be meaningful on -
    // variant_edit_all_tests covers the message path on loaded samples.
    scxt::engine::Zone::Variants vd;
    scxt::engine::Zone::applyVariantFieldEdit(
        vd, edited, offsetof(scxt::engine::Zone::SingleVariant, startSample),
        sizeof(edited.startSample), 0, false, true);
    REQUIRE(vd.variants[0].startSample == 12345);
}

TEST_CASE("Out of range part in a zone delta is refused")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.stepUI();

    auto &zone = th.engine->getPatch()->getPart(0)->getGroup(0)->getZone(0);
    auto keyStart = zone->mapping.keyboardRange.keyStart;

    // dim 0 is the keyboard range; the part index is the third element
    th.sendToSerialization(cmsg::ApplyZoneDelta({false, false, scxt::numParts, 0, 1, 0}));
    th.sendToSerialization(cmsg::ApplyZoneDelta({false, false, 4096, 0, 1, 0}));
    th.sendToSerialization(cmsg::ApplyZoneDelta({false, false, -1, 0, 1, 0}));
    th.stepUI();

    REQUIRE(zone->mapping.keyboardRange.keyStart == keyStart);
}
