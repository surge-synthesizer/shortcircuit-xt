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
#include "patch_io/patch_io.h"
#include "test_utils.h"

namespace cmsg = scxt::messaging::client;
namespace fs = std::filesystem;
using scxt::patch_io::SaveStyles;

namespace
{
struct SavedSampleFixture
{
    scxt::clients::console_ui::ConsoleHarness th;
    fs::path dir, sample, out;

    SavedSampleFixture()
    {
        dir = fs::temp_directory_path() / "scxt-missing-sample-save";
        fs::remove_all(dir);
        fs::create_directories(dir / "loaded");
        fs::create_directories(dir / "moved");
        out = dir / "out";
        fs::create_directories(out);

        sample = dir / "loaded" / "Beep.wav";
        fs::copy_file(samplePath("next/Beep.wav"), sample);

        th.start();
        th.stepUI();
        send(cmsg::AddSampleWithRange({sample.u8string(), 60, 48, 72, 0, 127}));
        REQUIRE(th.engine->getPatch()->getPart(0)->getGroups().size() == 1);
    }

    ~SavedSampleFixture()
    {
        std::error_code ec;
        fs::remove_all(dir, ec);
    }

    template <typename T> void send(const T &msg)
    {
        th.sendToSerialization(msg);
        th.stepUI(30);
    }

    void save(SaveStyles style, bool asPart, const std::string &name)
    {
        auto p = (out / name).u8string();
        if (asPart)
            send(cmsg::SavePart({p, 0, (int)style}));
        else
            send(cmsg::SaveMulti({p, (int)style}));
    }

    bool reportedMissingSamples()
    {
        for (const auto &err : th.editor->readErrors())
            if (std::get<1>(err) == "Missing Sample Files")
                return true;
        return false;
    }
};
} // namespace

TEST_CASE("Collecting and monolith saves write when samples are present", "[patch_io]")
{
    SavedSampleFixture f;

    SECTION("multi as monolith")
    {
        f.save(SaveStyles::AS_MONOLITH, false, "mono.scm");
        CHECK(fs::exists(f.out / "mono.scm"));
    }
    SECTION("part as monolith")
    {
        f.save(SaveStyles::AS_MONOLITH, true, "mono.scp");
        CHECK(fs::exists(f.out / "mono.scp"));
    }
    SECTION("multi with collected samples")
    {
        f.save(SaveStyles::WITH_COLLECTED_SAMPLES, false, "coll.scm");
        CHECK(fs::exists(f.out / "coll.scm"));
        CHECK(fs::exists(f.out / "coll Samples" / "Beep.wav"));
    }
    SECTION("part with collected samples")
    {
        f.save(SaveStyles::WITH_COLLECTED_SAMPLES, true, "coll.scp");
        CHECK(fs::exists(f.out / "coll.scp"));
        CHECK(fs::exists(f.out / "coll Samples" / "Beep.wav"));
    }
    SECTION("collect only")
    {
        f.save(SaveStyles::ONLY_COLLECT, false, "collected");
        CHECK(fs::exists(f.out / "collected" / "Beep.wav"));
    }
    SECTION("part as sfz")
    {
        f.save(SaveStyles::AS_SFZ, true, "exported.sfz");
        CHECK(fs::exists(f.out / "exported.sfz"));
    }

    CHECK_FALSE(f.th.editor->hasErrors());
}

TEST_CASE("Collecting and monolith saves of a moved sample error and write nothing", "[patch_io]")
{
    SavedSampleFixture f;
    fs::rename(f.sample, f.dir / "moved" / "Beep.wav");

    SECTION("multi as monolith") { f.save(SaveStyles::AS_MONOLITH, false, "mono.scm"); }
    SECTION("part as monolith") { f.save(SaveStyles::AS_MONOLITH, true, "mono.scp"); }
    SECTION("multi with collected samples")
    {
        f.save(SaveStyles::WITH_COLLECTED_SAMPLES, false, "coll.scm");
    }
    SECTION("part with collected samples")
    {
        f.save(SaveStyles::WITH_COLLECTED_SAMPLES, true, "coll.scp");
    }
    SECTION("collect only multi") { f.save(SaveStyles::ONLY_COLLECT, false, "collected"); }
    SECTION("collect only part") { f.save(SaveStyles::ONLY_COLLECT, true, "collected"); }
    SECTION("part as sfz") { f.save(SaveStyles::AS_SFZ, true, "exported.sfz"); }

    CHECK(f.reportedMissingSamples());
    CHECK(f.th.editor->readErrors().size() == 1);
    CHECK(fs::is_empty(f.out));
}

TEST_CASE("Saves without samples still write when a sample has moved", "[patch_io]")
{
    SavedSampleFixture f;
    fs::rename(f.sample, f.dir / "moved" / "Beep.wav");

    SECTION("multi")
    {
        f.save(SaveStyles::NO_SAMPLES, false, "plain.scm");
        CHECK(fs::exists(f.out / "plain.scm"));
    }
    SECTION("part")
    {
        f.save(SaveStyles::NO_SAMPLES, true, "plain.scp");
        CHECK(fs::exists(f.out / "plain.scp"));
    }

    CHECK_FALSE(f.th.editor->hasErrors());
}
