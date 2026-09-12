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

#include <vector>

#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "messaging/client/client_messages.h"
#include "test_utils.h"

namespace cmsg = scxt::messaging::client;

namespace
{
struct DropFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    DropFixture()
    {
        th.start();
        th.stepUI();
    }

    scxt::engine::Part &part() { return *th.engine->getPatch()->getPart(0); }

    size_t zonesIn(size_t group)
    {
        if (group >= part().getGroups().size())
            return 0;
        return part().getGroup(group)->getZones().size();
    }

    // adds land after an audio thread hop, and a drain is not a barrier
    bool drainUntilZones(size_t group, size_t n)
    {
        for (int i = 0; i < 300; ++i)
        {
            th.stepUI(1);
            if (zonesIn(group) == n)
            {
                th.stepUI(20);
                return true;
            }
        }
        return false;
    }

    void resetCounts()
    {
        th.editor->structureUpdateCount = 0;
        th.editor->selectionStateCount = 0;
        th.editor->receivedByteCount = 0;
    }
};

cmsg::addSampleSpec_t beep(int root, int keyLo, int keyHi)
{
    return {samplePath("next/Beep.wav").u8string(), root, keyLo, keyHi, 0, 127, false};
}

std::vector<cmsg::addSampleSpec_t> beeps(int n)
{
    return std::vector<cmsg::addSampleSpec_t>(n, beep(60, 48, 72));
}
} // namespace

TEST_CASE("A multi sample drop sends one structure and one selection update", "[drop]")
{
    DropFixture f;
    f.resetCounts();

    f.th.sendToSerialization(cmsg::AddSamples({beeps(50), -1, -1}));
    REQUIRE(f.drainUntilZones(0, 50));

    REQUIRE(f.th.editor->structureUpdateCount == 1);
    REQUIRE(f.th.editor->selectionStateCount == 1);
}

TEST_CASE("A multi sample drop costs about what a single sample drop costs", "[drop]")
{
    size_t oneFile{0}, manyFiles{0};
    {
        DropFixture f;
        f.resetCounts();
        f.th.sendToSerialization(cmsg::AddSamples({beeps(1), -1, -1}));
        REQUIRE(f.drainUntilZones(0, 1));
        oneFile = f.th.editor->receivedByteCount;
    }
    {
        DropFixture f;
        f.resetCounts();
        f.th.sendToSerialization(cmsg::AddSamples({beeps(50), -1, -1}));
        REQUIRE(f.drainUntilZones(0, 50));
        manyFiles = f.th.editor->receivedByteCount;
    }
    REQUIRE(oneFile > 0);
    REQUIRE(manyFiles < 2 * oneFile);
}

TEST_CASE("A multi sample drop gives each zone its own mapping", "[drop]")
{
    DropFixture f;

    std::vector<cmsg::addSampleSpec_t> specs;
    for (int i = 0; i < 5; ++i)
        specs.push_back(beep(40 + i, 40 + i, 40 + i));

    f.th.sendToSerialization(cmsg::AddSamples({specs, -1, -1}));
    REQUIRE(f.drainUntilZones(0, 5));

    const auto &zones = f.part().getGroup(0)->getZones();
    for (int i = 0; i < 5; ++i)
    {
        INFO("zone " << i);
        REQUIRE(zones[i]->mapping.rootKey == 40 + i);
        REQUIRE(zones[i]->mapping.keyboardRange.keyStart == 40 + i);
        REQUIRE(zones[i]->mapping.keyboardRange.keyEnd == 40 + i);
    }
}

TEST_CASE("A multi sample drop into an explicit group lands in that group", "[drop]")
{
    DropFixture f;
    for (int i = 0; i < 3; ++i)
        f.th.sendToSerialization(cmsg::CreateGroup(0));
    f.th.stepUI(30);
    REQUIRE(f.part().getGroups().size() == 3);

    // the middle group is neither the first nor the most recently created one
    f.th.sendToSerialization(cmsg::AddSamples({beeps(4), 0, 1}));
    REQUIRE(f.drainUntilZones(1, 4));

    REQUIRE(f.zonesIn(0) == 0);
    REQUIRE(f.zonesIn(2) == 0);
}

TEST_CASE("A multi sample drop is a single undo step", "[drop][undo]")
{
    DropFixture f;
    auto baseSize = f.th.engine->undoManager.undoStackSize();

    f.th.sendToSerialization(cmsg::AddSamples({beeps(3), -1, -1}));
    REQUIRE(f.drainUntilZones(0, 3));
    REQUIRE(f.th.engine->undoManager.undoStackSize() == baseSize + 1);

    f.th.sendToSerialization(cmsg::Undo(true));
    f.th.stepUI(30);
    REQUIRE(f.part().getGroups().empty());
}
