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

#include <string>

#include "utils.h"
#include "filesystem/import.h"

TEST_CASE("Guarantee Extension")
{
    using scxt::guaranteeExtension;

    SECTION("Adds a missing extension")
    {
        // Issue 2442: the linux save dialog returns the typed name verbatim
        REQUIRE(guaranteeExtension(fs::path{"FooBaz"}, ".scm").u8string() == "FooBaz.scm");
        REQUIRE(guaranteeExtension(fs::path{"/a/b/FooBaz"}, ".scm").u8string() ==
                "/a/b/FooBaz.scm");
    }

    SECTION("Leaves an extension it already has alone")
    {
        REQUIRE(guaranteeExtension(fs::path{"FooBaz.scm"}, ".scm").u8string() == "FooBaz.scm");
        // and doesn't double up on a differently cased one
        REQUIRE(guaranteeExtension(fs::path{"FooBaz.SCM"}, ".scm").u8string() == "FooBaz.SCM");
        REQUIRE(guaranteeExtension(fs::path{"FooBaz.Scm"}, ".scm").u8string() == "FooBaz.Scm");
    }

    SECTION("Takes the extension in any of the three spellings")
    {
        REQUIRE(guaranteeExtension(fs::path{"FooBaz"}, "scm").u8string() == "FooBaz.scm");
        REQUIRE(guaranteeExtension(fs::path{"FooBaz"}, ".scm").u8string() == "FooBaz.scm");
        REQUIRE(guaranteeExtension(fs::path{"FooBaz"}, "*.scm").u8string() == "FooBaz.scm");
    }

    SECTION("Appends rather than replacing, so a dotted name survives")
    {
        // replace_extension would turn this into "Bass 3.scm" and lose the .2
        REQUIRE(guaranteeExtension(fs::path{"Bass 3.2"}, ".scm").u8string() == "Bass 3.2.scm");
        REQUIRE(guaranteeExtension(fs::path{"Kick.wav"}, ".scm").u8string() == "Kick.wav.scm");
    }

    SECTION("Handles each of the formats we save")
    {
        REQUIRE(guaranteeExtension(fs::path{"P"}, "*.scm").u8string() == "P.scm");
        REQUIRE(guaranteeExtension(fs::path{"P"}, "*.scp").u8string() == "P.scp");
        REQUIRE(guaranteeExtension(fs::path{"P"}, "*.sfz").u8string() == "P.sfz");
        REQUIRE(guaranteeExtension(fs::path{"P"}, "*.scmod").u8string() == "P.scmod");
        REQUIRE(guaranteeExtension(fs::path{"P"}, "*.sctheme").u8string() == "P.sctheme");
        REQUIRE(guaranteeExtension(fs::path{"P"}, "*.vcfx").u8string() == "P.vcfx");
        REQUIRE(guaranteeExtension(fs::path{"P"}, "*.busfx").u8string() == "P.busfx");
    }

    SECTION("Won't make a path out of nothing")
    {
        REQUIRE(guaranteeExtension(fs::path{}, ".scm").u8string().empty());
        // a trailing separator means no filename to extend
        REQUIRE(guaranteeExtension(fs::path{"/a/b/"}, ".scm").u8string() == "/a/b/");
        // and an empty extension is a no-op rather than a trailing dot
        REQUIRE(guaranteeExtension(fs::path{"FooBaz"}, "").u8string() == "FooBaz");
        REQUIRE(guaranteeExtension(fs::path{"FooBaz"}, "*").u8string() == "FooBaz");
        REQUIRE(guaranteeExtension(fs::path{"FooBaz"}, ".").u8string() == "FooBaz");
    }
}
