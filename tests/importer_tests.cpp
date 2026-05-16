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
    // One <group> header in the SFZ file, five <region> headers under it.
    REQUIRE(part.getGroups().size() == 1);
    auto &group = part.getGroups()[0];
    REQUIRE(group->getZones().size() == 5);

    // Group-level ampeg_sustain=53 applies to all regions via merged opcodes;
    // the helper converts percent → 0..1 level.
    for (auto &z : group->getZones())
        CHECK(z->egStorage[0].s == Approx(0.53f));
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

    // At least one zone should have rootKey set to something other than the
    // Zone default of 60 — verifies the SF2 importer wrote the rootKey field
    // (catches the kind of `60 + ... - 60` regression we just cleaned up).
    bool anyNonDefaultRoot = false;
    for (auto &g : part.getGroups())
        for (auto &z : g->getZones())
            if (z->mapping.rootKey != 60)
                anyNonDefaultRoot = true;
    CHECK(anyNonDefaultRoot);
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

    // At least one zone has a non-default keyboard range — verifies that the
    // AKAI importer reaches `importZoneMapping` and populates the range.
    bool anyMappedRange = false;
    for (auto &g : part.getGroups())
        for (auto &z : g->getZones())
            if (z->mapping.keyboardRange.keyEnd > 0)
                anyMappedRange = true;
    CHECK(anyMappedRange);
}

// Skipped when the Logic Sampler Instruments aren't installed (e.g. on CI or a
// machine without Logic). When the file IS present, this test exercises the
// EXS importer against a known crashy patch — the referenced AIFFs (e.g.
// Bellsp36.aif) declare more sample frames in the COMM chunk than fit in
// the SSND chunk, and load_data_i16BE in load_aiff.cpp reads past the
// buffer end without bounds checking. SIGSEGV.
TEST_CASE("Import EXS Logic library fixture (skipped if absent)", "[importer][local-only]")
{
    auto p = fs::path("/Library/Application Support/Logic/Sampler Instruments/"
                      "05 Synthesizers/02 Synth Pads/Bellsphere Dark Filter.exs");
    if (!fs::exists(p))
    {
        WARN("Skipping Logic factory EXS test — fixture not present at " << p.string());
        SUCCEED("Logic factory EXS not installed on this machine");
        return;
    }

    INFO("fixture=" << p.string());
    ImporterFixture f;
    f.loadSample(p);

    auto &part = f.part0();
    REQUIRE(part.getGroups().size() >= 1);
    int totalZones = 0;
    for (auto &g : part.getGroups())
        totalZones += (int)g->getZones().size();
    CHECK(totalZones >= 1);
}

TEST_CASE("Import EXS fixture", "[importer]")
{
    auto p = fixturePath("exs/Numbers.exs");
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

    // At least one zone has a non-default keyboard range — verifies that the
    // EXS importer reaches `importZoneMapping` and populates the range.
    bool anyMappedRange = false;
    for (auto &g : part.getGroups())
        for (auto &z : g->getZones())
            if (z->mapping.keyboardRange.keyEnd > 0)
                anyMappedRange = true;
    CHECK(anyMappedRange);
}

TEST_CASE("Import multisample fixture", "[importer]")
{
    auto p = fixturePath("scxt_test.multisample");
    INFO("fixture=" << p.string());
    REQUIRE(fs::exists(p));

    ImporterFixture f;
    f.loadSample(p);

    auto &part = f.part0();
    REQUIRE(part.getGroups().size() == 1);

    auto &group = part.getGroups()[0];
    REQUIRE(group->getZones().size() == 2);

    auto &z0 = group->getZones()[0];
    CHECK(z0->mapping.rootKey == 34);
    CHECK(z0->mapping.keyboardRange.keyStart == 33);
    CHECK(z0->mapping.keyboardRange.keyEnd == 35);
    CHECK(z0->mapping.velocityRange.velStart == 0);
    CHECK(z0->mapping.velocityRange.velEnd == 39);

    auto &z1 = group->getZones()[1];
    CHECK(z1->mapping.rootKey == 37);
    CHECK(z1->mapping.keyboardRange.keyStart == 36);
    CHECK(z1->mapping.keyboardRange.keyEnd == 58);
    CHECK(z1->mapping.velocityRange.velStart == 0);
    CHECK(z1->mapping.velocityRange.velEnd == 39);
}
