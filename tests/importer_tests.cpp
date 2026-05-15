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

#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"

#ifndef SCXT_TEST_SOURCE_DIR
#define SCXT_TEST_SOURCE_DIR ""
#endif

/*
 * Regression fence for the multi-format importers. Each test sends
 * c2s_add_sample with a fixture path through the real serialization
 * thread, which routes to the appropriate importer via
 * Engine::loadSampleIntoSelectedPartAndGroup. The drain count is
 * generous (50 steps) because compound importers go through
 * stopAudioThreadThenRunOnSerial which has to suspend and restart the
 * audio thread.
 *
 * Multisample is intentionally omitted — we don't currently have a
 * checked-in fixture for it. Add one and a TEST_CASE here when ready.
 */

namespace cmsg = scxt::messaging::client;
namespace fs = std::filesystem;

namespace
{

struct ImporterFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    ImporterFixture()
    {
        th.start();
        th.stepUI();
    }

    scxt::engine::Engine &engine() { return *th.engine; }
    scxt::engine::Part &part0() { return *th.engine->getPatch()->getPart(0); }

    void loadSample(const fs::path &p, size_t drainSteps = 50)
    {
        th.sendToSerialization(cmsg::AddSample(p.u8string()));
        th.stepUI(drainSteps);
    }
};

fs::path fixturePath(const std::string &relative)
{
    return fs::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / relative;
}

} // namespace

TEST_CASE("Import SFZ fixture", "[importer]")
{
    auto p = fixturePath("malicex_sfz/YM-FM_Font Music Box.sfz");
    INFO("fixture=" << p.string());
    REQUIRE(fs::exists(p));

    ImporterFixture f;
    f.loadSample(p);

    auto &part = f.part0();
    REQUIRE(part.getGroups().size() >= 1);

    int totalZones = 0;
    for (auto &g : part.getGroups())
        totalZones += (int)g->getZones().size();
    REQUIRE(totalZones >= 1);
}

TEST_CASE("Import SF2 fixture", "[importer]")
{
    auto p = fixturePath("Crysrhod.sf2");
    INFO("fixture=" << p.string());
    REQUIRE(fs::exists(p));

    ImporterFixture f;
    f.loadSample(p);

    auto &part = f.part0();
    REQUIRE(part.getGroups().size() >= 1);

    int totalZones = 0;
    for (auto &g : part.getGroups())
        totalZones += (int)g->getZones().size();
    REQUIRE(totalZones >= 1);
}

TEST_CASE("Import AKAI fixture", "[importer]")
{
    auto p = fixturePath("akai_s6k/POWER SECT S.AKP");
    INFO("fixture=" << p.string());
    REQUIRE(fs::exists(p));

    ImporterFixture f;
    f.loadSample(p);

    auto &part = f.part0();
    REQUIRE(part.getGroups().size() >= 1);

    int totalZones = 0;
    for (auto &g : part.getGroups())
        totalZones += (int)g->getZones().size();
    REQUIRE(totalZones >= 1);
}
