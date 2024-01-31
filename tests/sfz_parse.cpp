/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
 * transaction log.
 *
 * ShortcircuitXT is released under the Gnu General Public Licence
 * V3 or later (GPL-3.0-or-later). The license is found in the file
 * "LICENSE" in the root of this repository or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Individual sections of code which comprises ShortcircuitXT in this
 * repository may also be used under an MIT license. Please see the
 * section  "Licensing" in "README.md" for details.
 *
 * ShortcircuitXT is inspired by, and shares code with, the
 * commercial product Shortcircuit 1 and 2, released by VemberTech
 * in the mid 2000s. The code for Shortcircuit 2 was opensourced in
 * 2020 at the outset of this project.
 *
 * All source for ShortcircuitXT is available at
 * https://github.com/surge-synthesizer/shortcircuit-xt
 */

#include "catch2/catch2.hpp"
#include "sfz_support/sfz_parse.h"

TEST_CASE("SFZ Tokens", "[sfz]")
{
    SECTION("Just a Comment")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ = "/* Here's a region */\n";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 0);
    }
    SECTION("Just a Comment")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ = "// Here's a region\n";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 0);
    }
    SECTION("Two Comments")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ = "// Here's a region\n// Which is fun\n";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 0);
    }
    SECTION("Just a global")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ = "<global>";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 1);
        REQUIRE(res[0].first.name == "global");
        REQUIRE(res[0].first.type == scxt::sfz_support::SFZParser::Header::global);
    }

    SECTION("Grr No Space Comments")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ =
            "<region>key= /*hey*/ foo=17 bar=hootie jim=/*hi*/ jon=thisthat/*bar*/ fred=21//yay";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 1);
    }

    SECTION("Just a Region and a Comment")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ = R"SFZ(
// Here's a region
<region>key=70 blank= /* hey */ foo=myThingy/*Yeah */
)SFZ";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 1);
        auto &top = res[0];
        REQUIRE(top.first.type == scxt::sfz_support::SFZParser::Header::region);

        REQUIRE(res[0].second.size() == 3);
        auto &opcodes = res[0].second;

        REQUIRE(opcodes[0].name == "key");
        REQUIRE((opcodes[0].value) == "70");
        REQUIRE(opcodes[1].name == "blank");
        REQUIRE(opcodes[2].name == "foo");
        REQUIRE((opcodes[2].value) == "myThingy");
    }

    SECTION("A Region with two keywords")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ = "<region>key=50 sample=d4.wav";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 1);
        REQUIRE(res[0].second.size() == 2);
        auto &kv = res[0].second;
        REQUIRE(kv[0].name == "key");
        REQUIRE((kv[0].value) == "50");
        REQUIRE(kv[1].name == "sample");
        REQUIRE((kv[1].value) == "d4.wav");
    }

    SECTION("A Region with two blank keywords")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ = "<region>key= sample=";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 1);
        REQUIRE(res[0].second.size() == 2);
    }

    SECTION("A Region with a comment")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ = "<region>key=/* Nothin */ foo=bar/*yeah*/ jim=1.23//hooey";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 1);
        REQUIRE(res[0].second.size() == 3);
    }

    SECTION("Values have Spaces. Because of course they do.")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ =
            "<region>sample=foo bar.wav this=12\n<region>sample=jim bob.wav// yup\n";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 2);
        REQUIRE(res[0].second.size() == 2);
        REQUIRE((res[0].second[0].value) == "foo bar.wav");
        REQUIRE(res[1].second.size() == 1);
        REQUIRE((res[1].second[0].value) == "jim bob.wav");
    }

    SECTION("SFZFormat.com template")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ = R"SFZ(

//------------------------------------------------------------------------------
// A basic sfz template
//------------------------------------------------------------------------------
<control>
default_path= // relative path of your samples

<global>
// parameters that affect the whole instrument go here.

// *****************************************************************************
// Your mapping starts here
// *****************************************************************************

<group> // 1

// Parameters that affect multiple regions go here

  fil_type=         // One of the many filter types available
  cutoff=           // freq in hertz
  cutoff_onccX=     // variation in cents
  resonance=        // value in db
  resonance_onccX=  // variation in db

  trigger=attack    // or release or first or legato
  loop_mode=no_loop // or loop_continuous or one_shot or loop_sustain

<region> sample=/*wav or flac file*/ key=// or lokey= hikey= pitch_keycenter=
<region> sample= key=
<region> sample= key=
<region> sample= key=
<region> sample= key=
<region> sample= key=
<region> sample= key=
<region> sample= key=
)SFZ";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 11);
        REQUIRE(res[2].second.size() == 7);
    }
    SECTION("Single Slash Comments")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ = std::string() + "/ This is a comment\n" +
                            "<region>key=3 foo=bar/ ueahy\n" +
                            "sample=foo/bar/x.wav\n"
                            "<region>\n" +
                            "sample=foo/bar/y.wav// Does this work\n";

        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 2);
        REQUIRE(res[0].second.size() == 3);
        REQUIRE(res[0].second[1].value == "bar");
        REQUIRE(res[0].second[2].value == "foo/bar/x.wav");
        REQUIRE(res[1].second.size() == 1);
        REQUIRE(res[1].second[0].value == "foo/bar/y.wav");
    }

    SECTION("From The Wrench")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string s = R"SFZ(<region>
sample=the wrench (hard) Samples\whh1.wav
lokey=c0
hikey=g#6
pitch_keycenter=b4
tune=-45)SFZ";
        auto res = p.parse(s);
        REQUIRE(res.size() == 1);
        auto &k = res[0].second;
        REQUIRE(k[0].name == "sample");
        REQUIRE(k[0].value == "the wrench (hard) Samples\\whh1.wav");
    }

    SECTION("More Advanced")
    {
        auto p = scxt::sfz_support::SFZParser();

        std::string anSFZ = R"SFZ(
// A sample SFZ copied from sfzformat.com
<global>ampeg_release=0.3 amp_veltrack=0 sw_lokey=48 sw_hikey=49
custom=/*we don't need this*/ another= // thats blank
group=1 off_by=1 off_mode=normal
<group>lokey=50 hikey=51 pitch_keycenter=50 sw_last=48
<region>sample=d4_p.wav xfin_locc1=0 xfin_hicc1=42 xfout_locc1=43 xfout_hicc1=85
<region>sample=d4_mf.wav xfin_locc1=43 xfin_hicc1=85 xfout_locc1=86 xfout_hicc1=127
<region>sample=d4_f.wav xfin_locc1=86 xfin_hicc1=127
<group>lokey=52 hikey=53 pitch_keycenter=52 sw_last=48
<region>sample=e4_p.wav xfin_locc1=0 xfin_hicc1=42 xfout_locc1=43 xfout_hicc1=85
<region>sample=e4_mf.wav xfin_locc1=43 xfin_hicc1=85 xfout_locc1=86 xfout_hicc1=127
<region>sample=e4_f.wav xfin_locc1=86 xfin_hicc1=127
<group>lokey=50 hikey=51 pitch_keycenter=50 sw_last=49
<region>sample=d4_ft_p.wav xfin_locc1=0 xfin_hicc1=63 xfout_locc1=64 xfout_hicc1=127
<region>sample=d4_ft_f.wav xfin_locc1=64 xfin_hicc1=127
<group>lokey=52 hikey=53 pitch_keycenter=52 sw_last=49
<region>sample=e4_ft_p.wav xfin_locc1=0 xfin_hicc1=63 xfout_locc1=64 xfout_hicc1=127
<region>sample=e4_ft_f.wav xfin_locc1=64 xfin_hicc1=127
)SFZ";
        auto res = p.parse(anSFZ);
        REQUIRE(res.size() == 15);
    }
}