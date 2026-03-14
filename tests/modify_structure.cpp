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

namespace cmsg = scxt::messaging::client;
using ZoneAddress = scxt::selection::SelectionManager::ZoneAddress;

TEST_CASE("Duplicate Group")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    // Add two zones to the initial group (group 0, part 0)
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    th.stepUI();

    auto &part = th.engine->getPatch()->getPart(0);
    REQUIRE(part->getGroups().size() == 1);
    REQUIRE(part->getGroup(0)->getZones().size() == 2);
    std::string originalName = part->getGroup(0)->name;

    // Duplicate group 0
    th.sendToSerialization(cmsg::DuplicateGroup(ZoneAddress{0, 0, -1}));
    th.stepUI();

    // Should now have two groups
    REQUIRE(part->getGroups().size() == 2);

    // Duplicate should have all the same zones
    REQUIRE(part->getGroup(1)->getZones().size() == 2);

    // Duplicate should have a "(copy)" suffix in the name
    REQUIRE(part->getGroup(1)->name == originalName + " (copy)");
}

TEST_CASE("Copy and Paste Group")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    // Add zones to the initial group
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 73, 84, 0, 127}));
    th.stepUI();

    auto &part = th.engine->getPatch()->getPart(0);
    REQUIRE(part->getGroups().size() == 1);
    REQUIRE(part->getGroup(0)->getZones().size() == 3);
    std::string originalName = part->getGroup(0)->name;

    // Clipboard should be empty initially
    REQUIRE(th.engine->clipboard.getClipboardType() ==
            scxt::engine::Clipboard::ContentType::NONE);

    // Copy the group
    th.sendToSerialization(cmsg::CopyGroup(ZoneAddress{0, 0, -1}));
    th.stepUI();

    // Clipboard should now hold a group
    REQUIRE(th.engine->clipboard.getClipboardType() ==
            scxt::engine::Clipboard::ContentType::GROUP);

    // Paste into part 0 (group address is where it will be added after)
    th.sendToSerialization(cmsg::PasteGroup(ZoneAddress{0, 0, -1}));
    th.stepUI();

    // Should now have two groups
    REQUIRE(part->getGroups().size() == 2);

    // Pasted group should have all the same zones
    REQUIRE(part->getGroup(1)->getZones().size() == 3);

    // Pasted group should have a "(Copy)" suffix
    REQUIRE(part->getGroup(1)->name == originalName + " (Copy)");

    // Paste again - should get a distinct name
    th.sendToSerialization(cmsg::PasteGroup(ZoneAddress{0, 0, -1}));
    th.stepUI();

    REQUIRE(part->getGroups().size() == 3);
    REQUIRE(part->getGroup(2)->name == originalName + " (Copy 2)");
}

TEST_CASE("Paste Group Does Nothing When Clipboard Holds Zone")
{
    scxt::clients::console_ui::ConsoleHarness th;
    th.start();
    th.stepUI();

    th.sendToSerialization(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    th.stepUI();

    auto &part = th.engine->getPatch()->getPart(0);
    REQUIRE(part->getGroups().size() == 1);

    // Copy a zone (not a group) so clipboard holds ZONE type
    th.sendToSerialization(cmsg::CopyZone(ZoneAddress{0, 0, 0}));
    th.stepUI();

    REQUIRE(th.engine->clipboard.getClipboardType() ==
            scxt::engine::Clipboard::ContentType::ZONE);

    // PasteGroup should be a no-op since clipboard holds a zone
    th.sendToSerialization(cmsg::PasteGroup(ZoneAddress{0, 0, -1}));
    th.stepUI();

    // Still one group
    REQUIRE(part->getGroups().size() == 1);
}
