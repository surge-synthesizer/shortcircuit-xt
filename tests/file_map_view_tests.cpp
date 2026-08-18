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
 * FileMapView is the bottom of the sample loading stack: every wav and aiff, and every md5
 * we take of one, goes through it. Its contract is narrow - either isMapped() and then data()
 * and dataSize() describe the whole file, or !isMapped() and the other two say nothing - and
 * the callers lean on it, so it is pinned here.
 *
 * The unhappy paths all used to arrive at !isMapped() by accident rather than by being
 * detected, so these cases largely passed before the error handling was tightened. They are
 * here to keep the contract from drifting, not to demonstrate the old bugs.
 */

#include "catch2/catch2.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "infrastructure/file_map_view.h"

#include "test_utils.h"

namespace fsys = std::filesystem;

using scxt::infrastructure::FileMapView;

TEST_CASE("A mapped file describes its whole contents", "[filemap]")
{
    auto p = samplePath("WavStereo48k.wav");
    REQUIRE(fsys::exists(p));

    FileMapView fmv(p);
    REQUIRE(fmv.isMapped());
    REQUIRE(fmv.data() != nullptr);
    REQUIRE(fmv.dataSize() == fsys::file_size(p));

    // the mapping has to be readable end to end, not just at the head
    auto *d = (const unsigned char *)fmv.data();
    REQUIRE(d[0] == 'R');
    REQUIRE(d[1] == 'I');
    REQUIRE(d[2] == 'F');
    REQUIRE(d[3] == 'F');

    size_t acc{0};
    for (size_t i = 0; i < fmv.dataSize(); ++i)
        acc += d[i];
    REQUIRE(acc > 0);
}

TEST_CASE("A file which is not there does not map", "[filemap]")
{
    auto p = samplePath("ThisSampleDoesNotExist.wav");
    REQUIRE(!fsys::exists(p));

    FileMapView fmv(p);
    REQUIRE(!fmv.isMapped());
    REQUIRE(fmv.data() == nullptr);
    REQUIRE(fmv.dataSize() == 0);
}

TEST_CASE("A directory does not map", "[filemap]")
{
    auto p = fsys::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples";
    REQUIRE(fsys::is_directory(p));

    FileMapView fmv(p);
    REQUIRE(!fmv.isMapped());
    REQUIRE(fmv.data() == nullptr);
    REQUIRE(fmv.dataSize() == 0);
}

TEST_CASE("An empty file does not map", "[filemap]")
{
    auto p = fsys::temp_directory_path() / "scxt_file_map_view_empty.bin";
    {
        std::ofstream mk(p, std::ios::binary | std::ios::trunc);
        REQUIRE(mk.good());
    }
    REQUIRE(fsys::exists(p));
    REQUIRE(fsys::file_size(p) == 0);

    {
        FileMapView fmv(p);
        REQUIRE(!fmv.isMapped());
        REQUIRE(fmv.dataSize() == 0);
    }

    fsys::remove(p);
}

TEST_CASE("A failed map does not hold its descriptor", "[filemap]")
{
    /*
     * A directory opens fine and only fails at the map, so it is the case which really has a
     * descriptor to give back. The count here is well past any default soft limit, so a leak
     * shows up as a map which suddenly cannot open a file that is really there.
     */
    auto dir = fsys::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples";
    for (int i = 0; i < 4096; ++i)
    {
        FileMapView fmv(dir);
        REQUIRE(!fmv.isMapped());
    }

    FileMapView good(samplePath("WavStereo48k.wav"));
    REQUIRE(good.isMapped());
}
