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
#include "infrastructure/user_defaults.h"
#include "patch_io/patch_io.h"
#include "selection/selection_manager.h"
#include "test_utils.h"

namespace cmsg = scxt::messaging::client;
using ZoneAddress = scxt::selection::SelectionManager::ZoneAddress;
using scxt::infrastructure::DefaultKeys;

namespace
{
struct StartupFixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    StartupFixture()
    {
        th.start();
        th.stepUI();
    }

    scxt::engine::Engine &engine() { return *th.engine; }
    scxt::infrastructure::DefaultsProvider &defaults() { return *th.engine->defaults; }

    template <typename T> void send(const T &msg, size_t drainSteps = 10)
    {
        th.sendToSerialization(msg);
        th.stepUI(drainSteps);
    }

    // zones only go into the selected part
    void addZone(int part, int16_t leaveSelected = 0)
    {
        send(cmsg::SelectPart((int16_t)part));
        send(cmsg::AddBlankZone({part, 0, 48, 60, 0, 127}));
        send(cmsg::SelectPart(leaveSelected));
    }

    size_t groupCount(int part) { return engine().getPatch()->getPart(part)->getGroups().size(); }

    std::string groupName(int part)
    {
        return engine().getPatch()->getPart(part)->getGroup(0)->name;
    }

    // overridden so a test never reads or clears the developer's own startup default
    void setStartup(const std::filesystem::path &p)
    {
        defaults().addOverride(DefaultKeys::startupPatchPath, p.u8string());
    }

    std::string startup()
    {
        return defaults().getUserDefaultValue(DefaultKeys::startupPatchPath, std::string("unset"));
    }

    bool hasStartupError()
    {
        for (const auto &e : th.editor->readErrors())
            if (std::get<1>(e) == "Startup Patch Unavailable")
                return true;
        return false;
    }
};

std::filesystem::path startupDir()
{
    auto dir = std::filesystem::temp_directory_path() / "scxt-startup-patch-tests";
    std::filesystem::create_directories(dir);
    return dir;
}

std::filesystem::path saveStartupMulti()
{
    auto mp = startupDir() / "startup.scm";
    StartupFixture src;
    src.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    src.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"StartupMulti"}}));
    src.addZone(3);
    src.send(cmsg::SaveMulti({mp.u8string(), (int)scxt::patch_io::SaveStyles::NO_SAMPLES}), 30);
    REQUIRE(std::filesystem::exists(mp));
    return mp;
}

std::filesystem::path saveStartupPart()
{
    auto pp = startupDir() / "startup.scp";
    StartupFixture src;
    src.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    src.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"StartupPart"}}));
    src.send(cmsg::SavePart({pp.u8string(), 0, (int)scxt::patch_io::SaveStyles::NO_SAMPLES}), 30);
    REQUIRE(std::filesystem::exists(pp));
    return pp;
}
} // namespace

TEST_CASE("Startup patch loads a multi", "[startup]")
{
    auto mp = saveStartupMulti();

    StartupFixture f;
    f.addZone(5);
    REQUIRE(f.groupCount(5) == 1);

    f.setStartup(mp);
    f.send(cmsg::ResetEngineToStartupPatch(false), 40);

    REQUIRE(f.groupCount(0) == 1);
    REQUIRE(f.groupName(0) == "StartupMulti");
    REQUIRE(f.groupCount(3) == 1);
    REQUIRE(f.groupCount(5) == 0);
    REQUIRE(!f.hasStartupError());
    REQUIRE(f.startup() == mp.u8string());

    std::filesystem::remove_all(startupDir());
}

TEST_CASE("Startup patch loads a part into part one of an empty engine", "[startup]")
{
    auto pp = saveStartupPart();

    StartupFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::AddBlankZone({0, 0, 61, 72, 0, 127}));
    f.addZone(2, 2);
    REQUIRE(f.groupCount(2) == 1);

    f.setStartup(pp);
    f.send(cmsg::ResetEngineToStartupPatch(false), 40);

    REQUIRE(f.groupCount(0) == 1);
    REQUIRE(f.groupName(0) == "StartupPart");
    REQUIRE(f.engine().getPatch()->getPart(0)->getGroup(0)->getZones().size() == 1);
    REQUIRE(f.groupCount(2) == 0);
    REQUIRE(!f.hasStartupError());
    REQUIRE(f.startup() == pp.u8string());

    std::filesystem::remove_all(startupDir());
}

TEST_CASE("Unavailable startup patch errors, empties and clears", "[startup]")
{
    StartupFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.addZone(4);
    REQUIRE(f.groupCount(0) == 1);

    auto tmp = std::filesystem::temp_directory_path();
    SECTION("missing multi") { f.setStartup(tmp / "scxt-no-such-startup.scm"); }
    SECTION("missing part") { f.setStartup(tmp / "scxt-no-such-startup.scp"); }
    SECTION("not a patch")
    {
        f.setStartup(std::filesystem::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" /
                     "harpsi.sf2");
    }

    f.send(cmsg::ResetEngineToStartupPatch(false), 40);

    REQUIRE(f.hasStartupError());
    REQUIRE(f.groupCount(0) == 0);
    REQUIRE(f.groupCount(4) == 0);
    REQUIRE(f.startup().empty());
    REQUIRE(f.defaults().hasOverride(DefaultKeys::startupPatchPath));
}

TEST_CASE("Unset startup patch leaves the engine alone", "[startup]")
{
    StartupFixture f;
    f.send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
    f.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"Untouched"}}));
    REQUIRE(f.engine().undoManager.hasUndoSteps());

    f.setStartup({});
    f.send(cmsg::ResetEngineToStartupPatch(true), 40);

    REQUIRE(f.groupCount(0) == 1);
    REQUIRE(f.groupName(0) == "Untouched");
    REQUIRE(f.engine().undoManager.hasUndoSteps());
    REQUIRE(!f.hasStartupError());
}

TEST_CASE("Startup patch with collected samples loads before audio runs", "[startup]")
{
    namespace fs = std::filesystem;
    auto dir = fs::temp_directory_path() / "scxt-startup-collected";
    fs::remove_all(dir);
    fs::create_directories(dir / "loaded");
    fs::create_directories(dir / "multi");
    fs::create_directories(dir / "part");
    auto sample = dir / "loaded" / "Beep.wav";
    fs::copy_file(samplePath("next/Beep.wav"), sample);

    auto multi = dir / "multi" / "collected.scm";
    auto part = dir / "part" / "collected.scp";
    {
        StartupFixture src;
        src.send(cmsg::AddSampleWithRange({sample.u8string(), 60, 48, 72, 0, 127}), 30);
        REQUIRE(src.groupCount(0) == 1);
        src.send(cmsg::RenameGroup({ZoneAddress{0, 0, -1}, std::string{"Collected"}}));
        auto style = (int)scxt::patch_io::SaveStyles::WITH_COLLECTED_SAMPLES;
        src.send(cmsg::SaveMulti({multi.u8string(), style}), 30);
        src.send(cmsg::SavePart({part.u8string(), 0, style}), 30);
    }
    REQUIRE(fs::exists(multi));
    REQUIRE(fs::exists(part));
    // only the collected copies remain, so they must resolve relative to the patch
    fs::remove_all(dir / "loaded");

    fs::path startup;
    SECTION("multi") { startup = multi; }
    SECTION("part") { startup = part; }

    // a new plugin instance loads before the host starts processing
    auto e = std::make_unique<scxt::engine::Engine>();
    {
        auto bypass = e->getMessageController()->threadingChecker.bypassChecksInScope();
        REQUIRE(!e->getMessageController()->isAudioRunning);

        e->defaults->addOverride(DefaultKeys::startupPatchPath, startup.u8string());
        REQUIRE(scxt::patch_io::initFromStartupPatch(*e));

        const auto &pt = e->getPatch()->getPart(0);
        REQUIRE(pt->getGroups().size() == 1);
        REQUIRE(pt->getGroup(0)->name == "Collected");
        const auto &zone = pt->getGroup(0)->getZone(0);
        REQUIRE(zone->samplePointers[0]);
        REQUIRE(!zone->samplePointers[0]->isMissingPlaceholder);
    }
    e.reset();
    fs::remove_all(dir);
}
