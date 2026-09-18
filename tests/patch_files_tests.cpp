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
#include <fstream>

#include "browser/browser.h"

#include "console_harness.h"
#include "engine/engine.h"
#include "json/stream.h"
#include "messaging/client/client_messages.h"
#include "patch_io/patch_io.h"
#include "selection/selection_manager.h"

namespace cmsg = scxt::messaging::client;
namespace pio = scxt::patch_io;

namespace
{
struct FilesFixture
{
    scxt::clients::console_ui::ConsoleHarness th;
    std::filesystem::path dir;

    FilesFixture(const std::string &named)
    {
        dir = std::filesystem::temp_directory_path() / ("scxt_patch_files_" + named);
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
        th.start();
        th.stepUI();
    }
    ~FilesFixture() { std::filesystem::remove_all(dir); }

    scxt::engine::Engine &engine() { return *th.engine; }
    scxt::selection::SelectionManager::PatchFiles files()
    {
        return th.engine->getSelectionManager()->getPatchFiles();
    }

    template <typename T> void send(const T &msg, size_t drainSteps = 10)
    {
        th.sendToSerialization(msg);
        th.stepUI(drainSteps);
    }
};

void activate(FilesFixture &f, int16_t part)
{
    auto cfg = f.engine().getPatch()->getPart(part)->configuration;
    cfg.active = true;
    f.send(cmsg::UpdatePartFullConfig({part, cfg}), 20);
}
} // namespace

TEST_CASE("A name is sanitized into a filename", "[patchfiles]")
{
    REQUIRE(scxt::sanitizeFilename("Cello Sustain") == "Cello Sustain");
    REQUIRE(scxt::sanitizeFilename("AC/DC: Riff?") == "ACDC Riff");
    REQUIRE(scxt::sanitizeFilename("  padded  ") == "padded");
    REQUIRE(scxt::sanitizeFilename("...").empty());
    REQUIRE(scxt::sanitizeFilename("").empty());
    // an inner dot is a legal filename character
    REQUIRE(scxt::sanitizeFilename("Kit v1.2") == "Kit v1.2");
}

TEST_CASE("Stepping through the patches in a folder", "[patchfiles]")
{
    namespace br = scxt::browser;
    auto dir = std::filesystem::temp_directory_path() / "scxt_patch_files_jog";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);

    auto touch = [&dir](const std::string &n) {
        std::ofstream f(dir / n);
        f << "x";
        return dir / n;
    };

    SECTION("An empty or single-file folder has nowhere to go")
    {
        REQUIRE(br::Browser::patchFilesIn(dir, ".scm").empty());
        auto only = touch("Only.scm");
        auto one = br::Browser::patchFilesIn(dir, ".scm");
        REQUIRE(one.size() == 1);
        REQUIRE(!br::Browser::stepPatchFile(one, only, 1).has_value());
        REQUIRE(!br::Browser::stepPatchFile(one, only, -1).has_value());
    }

    SECTION("Stepping wraps at both ends and ignores other extensions")
    {
        auto b = touch("bravo.scm");
        auto a = touch("Alpha.scm");
        auto c = touch("Charlie.scm");
        touch("NotThis.scp");

        auto files = br::Browser::patchFilesIn(dir, ".scm");
        REQUIRE(files.size() == 3);
        // case-insensitive order: Alpha, bravo, Charlie
        REQUIRE(files[0] == a);
        REQUIRE(files[1] == b);
        REQUIRE(files[2] == c);

        REQUIRE(*br::Browser::stepPatchFile(files, a, 1) == b);
        REQUIRE(*br::Browser::stepPatchFile(files, c, 1) == a);
        REQUIRE(*br::Browser::stepPatchFile(files, a, -1) == c);

        // the file we were on has gone; land at the end we were heading for
        REQUIRE(*br::Browser::stepPatchFile(files, dir / "Deleted.scm", 1) == a);
        REQUIRE(*br::Browser::stepPatchFile(files, dir / "Deleted.scm", -1) == c);
    }

    SECTION("A folder that isn't there is empty, not an error")
    {
        REQUIRE(br::Browser::patchFilesIn(dir / "nope", ".scm").empty());
        REQUIRE(br::Browser::patchFilesIn({}, ".scm").empty());
    }

    std::filesystem::remove_all(dir);
}

TEST_CASE("Saving a part names it for the file", "[patchfiles]")
{
    FilesFixture f("save_part");
    auto tmp = f.dir / "Cello Sustain.scp";

    f.send(cmsg::SavePart({tmp.u8string(), 0, pio::SaveStyles::NO_SAMPLES}), 20);
    REQUIRE(std::filesystem::exists(tmp));

    REQUIRE(std::string(f.engine().getPatch()->getPart(0)->names.name) == "Cello Sustain");
    REQUIRE(f.files().parts[0].path == tmp);
    REQUIRE(!f.files().parts[0].monolith);
}

TEST_CASE("Loading a part names the slot for the file", "[patchfiles]")
{
    auto dir = std::filesystem::temp_directory_path() / "scxt_patch_files_load_part";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    auto tmp = dir / "Viola Short.scp";

    {
        FilesFixture src("load_part_src");
        src.send(cmsg::SavePart({tmp.u8string(), 0, pio::SaveStyles::NO_SAMPLES}), 20);
    }
    REQUIRE(std::filesystem::exists(tmp));

    FilesFixture f("load_part_dst");
    f.send(cmsg::LoadPartInto({tmp.u8string(), (int16_t)5}), 20);

    REQUIRE(std::string(f.engine().getPatch()->getPart(5)->names.name) == "Viola Short");
    REQUIRE(f.files().parts[5].path == tmp);
    // the other slots are untouched
    REQUIRE(f.files().parts[0].path.empty());

    std::filesystem::remove_all(dir);
}

TEST_CASE("Saving and loading a multi names it for the file", "[patchfiles]")
{
    auto dir = std::filesystem::temp_directory_path() / "scxt_patch_files_multi";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    auto tmp = dir / "String Quartet.scm";

    {
        FilesFixture src("multi_src");
        REQUIRE(src.files().multiName == "Default Multi");
        src.send(cmsg::SaveMulti({tmp.u8string(), pio::SaveStyles::NO_SAMPLES}), 20);
        REQUIRE(src.files().multiName == "String Quartet");
        REQUIRE(src.files().multi.path == tmp);
    }
    REQUIRE(std::filesystem::exists(tmp));

    FilesFixture f("multi_dst");
    f.send(cmsg::LoadMulti(tmp.u8string()), 30);
    REQUIRE(f.files().multiName == "String Quartet");
    REQUIRE(f.files().multi.path == tmp);

    std::filesystem::remove_all(dir);
}

TEST_CASE("A multi load re-roots part files whose folder is gone", "[patchfiles]")
{
    auto srcDir = std::filesystem::temp_directory_path() / "scxt_patch_files_reroot_src";
    auto dstDir = std::filesystem::temp_directory_path() / "scxt_patch_files_reroot_dst";
    for (auto &d : {srcDir, dstDir})
    {
        std::filesystem::remove_all(d);
        std::filesystem::create_directories(d);
    }
    auto part = srcDir / "Cello.scp";
    auto multi = srcDir / "Quartet.scm";

    {
        FilesFixture src("reroot_writer");
        src.send(cmsg::SavePart({part.u8string(), 0, pio::SaveStyles::NO_SAMPLES}), 20);
        src.send(cmsg::SaveMulti({multi.u8string(), pio::SaveStyles::NO_SAMPLES}), 20);
        REQUIRE(src.files().parts[0].path == part);
    }

    // the multi and its part move somewhere else, as they would on another machine
    std::filesystem::rename(multi, dstDir / "Quartet.scm");
    std::filesystem::rename(part, dstDir / "Cello.scp");
    std::filesystem::remove_all(srcDir);

    FilesFixture f("reroot_loader");
    f.send(cmsg::LoadMulti((dstDir / "Quartet.scm").u8string()), 30);

    REQUIRE(f.files().multi.path == dstDir / "Quartet.scm");
    // the part keeps its filename and follows the multi
    REQUIRE(f.files().parts[0].path == dstDir / "Cello.scp");
    REQUIRE(std::string(f.engine().getPatch()->getPart(0)->names.name) == "Cello");

    std::filesystem::remove_all(dstDir);
}

TEST_CASE("A multi load leaves the screen where it was", "[patchfiles]")
{
    auto dir = std::filesystem::temp_directory_path() / "scxt_patch_files_view";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    auto tmp = dir / "Two Part.scm";

    {
        // a multi saved on the mixer, with part 1 selected and two parts live
        FilesFixture src("view_writer");
        activate(src, 1);
        src.send(cmsg::SelectPart(1), 20);
        src.send(cmsg::UpdateOtherTabSelection({"main_screen", "mixer"}), 20);
        src.send(cmsg::SaveMulti({tmp.u8string(), pio::SaveStyles::NO_SAMPLES}), 20);
    }

    SECTION("The screen, tab and part you were on all survive")
    {
        FilesFixture f("view_same_part");
        f.send(cmsg::UpdateOtherTabSelection({"main_screen", "play"}), 20);
        f.send(cmsg::UpdateOtherTabSelection({"multi.pgz", "group"}), 20);
        activate(f, 1);
        f.send(cmsg::SelectPart(1), 20);

        f.send(cmsg::LoadMulti(tmp.u8string()), 30);

        const auto &sm = f.engine().getSelectionManager();
        REQUIRE(sm->otherTabSelection["main_screen"] == "play");
        REQUIRE(sm->otherTabSelection["multi.pgz"] == "group");
        REQUIRE(sm->selectedPart == 1);
    }

    SECTION("A part the loaded multi doesn't have falls back")
    {
        FilesFixture f("view_gone_part");
        activate(f, 7);
        f.send(cmsg::SelectPart(7), 20);
        REQUIRE(f.engine().getSelectionManager()->selectedPart == 7);

        f.send(cmsg::LoadMulti(tmp.u8string()), 30);

        const auto &sm = f.engine().getSelectionManager();
        REQUIRE(f.engine().getPatch()->getPart(sm->selectedPart)->configuration.active);
        REQUIRE(sm->selectedPart != 7);
    }

    std::filesystem::remove_all(dir);
}

TEST_CASE("A reset names the template and clears its files", "[patchfiles]")
{
    FilesFixture f("reset");
    auto tmp = f.dir / "Something.scm";
    f.send(cmsg::SaveMulti({tmp.u8string(), pio::SaveStyles::NO_SAMPLES}), 20);
    REQUIRE(f.files().multiName == "Something");

    f.send(cmsg::ResetEngine("InitSynth.dat"), 30);
    REQUIRE(f.files().multiName == "Init Synth");
    REQUIRE(f.files().multi.path.empty());
    REQUIRE(std::string(f.engine().getPatch()->getPart(0)->names.name) == "Synth");
    REQUIRE(std::string(f.engine().getPatch()->getPart(1)->names.name) == "Default Part");

    f.send(cmsg::ResetEngine("InitSamplerMulti.dat"), 30);
    REQUIRE(f.files().multiName == "Init 16 Part");
    REQUIRE(std::string(f.engine().getPatch()->getPart(0)->names.name) == "Default Part");
}

TEST_CASE("Clearing a part forgets the file it came from", "[patchfiles]")
{
    FilesFixture f("clear");
    auto tmp = f.dir / "Kick.scp";
    f.send(cmsg::SavePart({tmp.u8string(), 0, pio::SaveStyles::NO_SAMPLES}), 20);
    REQUIRE(f.files().parts[0].path == tmp);

    f.send(cmsg::ClearPart(0), 20);
    REQUIRE(f.files().parts[0].path.empty());
    REQUIRE(std::string(f.engine().getPatch()->getPart(0)->names.name) == "Default Part");
}

TEST_CASE("Renaming a multi is undoable", "[patchfiles]")
{
    FilesFixture f("rename");
    REQUIRE(f.files().multiName == "Default Multi");

    f.send(cmsg::RenameMulti("Live Rig"), 20);
    REQUIRE(f.files().multiName == "Live Rig");

    f.send(cmsg::Undo(true), 20);
    REQUIRE(f.files().multiName == "Default Multi");

    f.send(cmsg::Redo(true), 20);
    REQUIRE(f.files().multiName == "Live Rig");
}

TEST_CASE("Patch files survive a DAW state round trip", "[patchfiles]")
{
    FilesFixture f("daw");
    auto part = f.dir / "Snare.scp";
    auto tmp = f.dir / "Session Kit.scm";
    f.send(cmsg::SavePart({part.u8string(), 0, pio::SaveStyles::NO_SAMPLES}), 20);
    f.send(cmsg::SaveMulti({tmp.u8string(), pio::SaveStyles::NO_SAMPLES}), 20);

    std::string state;
    {
        auto sg = scxt::engine::Engine::StreamGuard(scxt::engine::Engine::FOR_DAW);
        state = scxt::json::streamEngineState(f.engine());
    }

    // the unstream has to happen on the serial thread, as a DAW state load does
    FilesFixture g("daw_restore");
    g.send(cmsg::UnstreamEngineState(state), 30);

    REQUIRE(g.files().multiName == "Session Kit");
    REQUIRE(g.files().multi.path == tmp);
    REQUIRE(g.files().parts[0].path == part);

    // and the client is told, or the header comes back greyed out
    REQUIRE(g.th.editor->clientPatchFiles.multiName == "Session Kit");
    REQUIRE(g.th.editor->clientPatchFiles.multi.path == tmp);
    REQUIRE(g.th.editor->clientPatchFiles.parts[0].path == part);
}
