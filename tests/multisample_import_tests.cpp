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

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <miniz.h>

#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "sample/import_support/import_numeric.h"

#ifndef SCXT_TEST_SOURCE_DIR
#define SCXT_TEST_SOURCE_DIR ""
#endif

/*
 * The Bitwig/PreSonus .multisample importer, against the published spec at
 * https://github.com/bitwig/multisample. Fixtures are synthesized rather than
 * committed: a .multisample is a zip of one multisample.xml plus WAVs, so the
 * builder below writes both and a case costs a few lines of XML.
 */

namespace cmsg = scxt::messaging::client;
namespace fs = std::filesystem;

namespace
{

void putTag(std::vector<uint8_t> &b, const char *t) { b.insert(b.end(), t, t + 4); }
void putU16(std::vector<uint8_t> &b, uint16_t v)
{
    b.push_back(v & 0xFF);
    b.push_back((v >> 8) & 0xFF);
}
void putU32(std::vector<uint8_t> &b, uint32_t v)
{
    b.push_back(v & 0xFF);
    b.push_back((v >> 8) & 0xFF);
    b.push_back((v >> 16) & 0xFF);
    b.push_back((v >> 24) & 0xFF);
}

struct SmplLoop
{
    uint32_t start, end;
    bool bidirectional{false};
};

// Mono 16-bit PCM at 48k. loop, when set, emits a smpl chunk the WAV loader
// reads into sample metadata - which is what an explicit <loop> must override.
std::vector<uint8_t> makeWav(uint32_t nframes, const SmplLoop *loop = nullptr)
{
    std::vector<uint8_t> body;
    putTag(body, "WAVE");

    putTag(body, "fmt ");
    putU32(body, 16);
    putU16(body, 1);     // PCM
    putU16(body, 1);     // mono
    putU32(body, 48000); //
    putU32(body, 48000 * 2);
    putU16(body, 2);
    putU16(body, 16);

    putTag(body, "data");
    uint32_t dataBytes = nframes * 2;
    putU32(body, dataBytes);
    for (uint32_t i = 0; i < dataBytes; ++i)
        body.push_back((uint8_t)(i & 0xFF));
    if (dataBytes & 1)
        body.push_back(0);

    if (loop)
    {
        putTag(body, "smpl");
        putU32(body, 36 + 24);
        for (int i = 0; i < 3; ++i)
            putU32(body, 0); // manufacturer, product, sample period
        putU32(body, 60);    // dwMIDIUnityNote
        putU32(body, 0);     // pitch fraction
        putU32(body, 0);     // SMPTE format
        putU32(body, 0);     // SMPTE offset
        putU32(body, 1);     // cSampleLoops
        putU32(body, 0);     // cbSamplerData
        putU32(body, 0);     // dwIdentifier
        putU32(body, loop->bidirectional ? 1 : 0);
        putU32(body, loop->start);
        putU32(body, loop->end);
        putU32(body, 0); // fraction
        putU32(body, 0); // play count
    }

    std::vector<uint8_t> f;
    putTag(f, "RIFF");
    putU32(f, (uint32_t)body.size());
    f.insert(f.end(), body.begin(), body.end());
    return f;
}

struct WavSpec
{
    std::string name;
    uint32_t frames{1000};
    const SmplLoop *loop{nullptr};
};

// Writes <tmp>/<stem>.multisample containing multisample.xml and each WAV.
fs::path buildMultisample(const std::string &stem, const std::string &xml,
                          const std::vector<WavSpec> &wavs)
{
    auto dir = fs::temp_directory_path() / "scxt-multisample-tests";
    fs::create_directories(dir);
    auto p = dir / (stem + ".multisample");
    fs::remove(p);

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    REQUIRE(mz_zip_writer_init_file(&zip, p.u8string().c_str(), 0));

    REQUIRE(mz_zip_writer_add_mem(&zip, "multisample.xml", xml.data(), xml.size(),
                                  MZ_DEFAULT_COMPRESSION));
    for (const auto &w : wavs)
    {
        auto bytes = makeWav(w.frames, w.loop);
        REQUIRE(mz_zip_writer_add_mem(&zip, w.name.c_str(), bytes.data(), bytes.size(),
                                      MZ_DEFAULT_COMPRESSION));
    }

    REQUIRE(mz_zip_writer_finalize_archive(&zip));
    mz_zip_writer_end(&zip);
    return p;
}

struct Fixture
{
    scxt::clients::console_ui::ConsoleHarness th;

    Fixture()
    {
        th.start();
        th.stepUI();
    }

    scxt::engine::Part &part0() { return *th.engine->getPatch()->getPart(0); }

    void load(const fs::path &p)
    {
        th.sendToSerialization(cmsg::AddSample(p.u8string()));
        th.stepUI(50);
    }

    // The single zone of a single-sample fixture.
    scxt::engine::Zone &onlyZone()
    {
        auto &groups = part0().getGroups();
        REQUIRE(groups.size() == 1);
        REQUIRE(groups[0]->getZones().size() == 1);
        return *groups[0]->getZones()[0];
    }
};

// One <sample> wrapping the supplied attributes and children, in a one-group file.
std::string oneSample(const std::string &sampleAttrs, const std::string &children)
{
    return std::string(R"(<?xml version="1.0" encoding="UTF-8"?>
<multisample name="T">
   <generator>test</generator>
   <category>Test</category>
   <creator>test</creator>
   <description/>
   <keywords/>
   <group name="G"/>
   <sample file="A.wav" group="0" )") +
           sampleAttrs + ">" + children + "</sample>\n</multisample>";
}

} // namespace

TEST_CASE("multisample.xml not ending in whitespace still parses", "[importer][multisample]")
{
    // TiXmlDocument::Parse returns null when its trailing SkipWhiteSpace runs
    // off the end, so testing the return value rejects a valid document whose
    // last byte is '>'. Only Error() distinguishes that from a real failure.
    auto p = buildMultisample("no_trailing_ws", oneSample("", R"(<key root="60"/><velocity/>)"),
                              {{"A.wav", 1000}});
    REQUIRE(fs::file_size(p) > 0);

    Fixture f;
    f.load(p);

    REQUIRE(f.part0().getGroups().size() == 1);
}

TEST_CASE("multisample loop mode=loop loops for the life of the voice", "[importer][multisample]")
{
    auto p = buildMultisample("loop_fwd", oneSample("", R"(<key root="60"/><velocity/>
        <loop mode="loop" start="100" stop="500"/>)"),
                              {{"A.wav", 1000}});
    Fixture f;
    f.load(p);

    auto &v = f.onlyZone().variantData.variants[0];
    CHECK(v.loopActive);
    CHECK(v.loopMode == scxt::engine::Zone::LoopMode::LOOP_DURING_VOICE);
    CHECK(v.loopDirection == scxt::engine::Zone::LoopDirection::FORWARD_ONLY);
    CHECK(v.startLoop == 100);
    CHECK(v.endLoop == 500);
}

TEST_CASE("multisample loop mode=ping-pong alternates direction", "[importer][multisample]")
{
    auto p = buildMultisample("loop_pp", oneSample("", R"(<key root="60"/><velocity/>
        <loop mode="ping-pong" start="100" stop="500"/>)"),
                              {{"A.wav", 1000}});
    Fixture f;
    f.load(p);

    auto &v = f.onlyZone().variantData.variants[0];
    CHECK(v.loopActive);
    CHECK(v.loopDirection == scxt::engine::Zone::LoopDirection::ALTERNATE_DIRECTIONS);
}

TEST_CASE("multisample loop with no bounds spans the whole sample", "[importer][multisample]")
{
    // Spec: start defaults to 0, stop defaults to the file length.
    auto p = buildMultisample("loop_bare", oneSample("", R"(<key root="60"/><velocity/>
        <loop mode="loop"/>)"),
                              {{"A.wav", 1000}});
    Fixture f;
    f.load(p);

    auto &v = f.onlyZone().variantData.variants[0];
    CHECK(v.loopActive);
    CHECK(v.startLoop == 0);
    CHECK(v.endLoop == 1000);
}

TEST_CASE("multisample loop fade is a ratio of the loop length", "[importer][multisample]")
{
    // Spec: fade is "Multiply with (stop - start)". 0.25 * (500-100) == 100.
    auto p = buildMultisample("loop_fade", oneSample("", R"(<key root="60"/><velocity/>
        <loop mode="loop" start="100" stop="500" fade="0.25"/>)"),
                              {{"A.wav", 1000}});
    Fixture f;
    f.load(p);

    CHECK(f.onlyZone().variantData.variants[0].loopFade == 100);
}

TEST_CASE("multisample loop mode=off overrides a looped smpl chunk", "[importer][multisample]")
{
    // An explicit <loop> wins over the WAV's own metadata, including when it
    // says off - otherwise mode="off" could never be honoured.
    SmplLoop sl{200, 800};
    auto p = buildMultisample("loop_off", oneSample("", R"(<key root="60"/><velocity/>
        <loop mode="off"/>)"),
                              {{"A.wav", 1000, &sl}});
    Fixture f;
    f.load(p);

    CHECK_FALSE(f.onlyZone().variantData.variants[0].loopActive);
}

TEST_CASE("multisample with no loop element keeps the smpl chunk loop", "[importer][multisample]")
{
    SmplLoop sl{200, 800};
    auto p = buildMultisample("loop_absent", oneSample("", R"(<key root="60"/><velocity/>)"),
                              {{"A.wav", 1000, &sl}});
    Fixture f;
    f.load(p);

    auto &v = f.onlyZone().variantData.variants[0];
    CHECK(v.loopActive);
    CHECK(v.startLoop == 200);
}

TEST_CASE("multisample sample-start and sample-stop set the endpoints", "[importer][multisample]")
{
    auto p = buildMultisample("endpoints",
                              oneSample(R"(sample-start="120.000" sample-stop="640.000")",
                                        R"(<key root="60"/><velocity/>)"),
                              {{"A.wav", 1000}});
    Fixture f;
    f.load(p);

    auto &v = f.onlyZone().variantData.variants[0];
    CHECK(v.startSample == 120);
    CHECK(v.endSample == 640);
}

TEST_CASE("multisample reverse attribute sets reverse playback", "[importer][multisample]")
{
    auto p = buildMultisample("reverse",
                              oneSample(R"(reverse="true")", R"(<key root="60"/><velocity/>)"),
                              {{"A.wav", 1000}});
    Fixture f;
    f.load(p);

    CHECK(f.onlyZone().variantData.variants[0].playReverse);
}

TEST_CASE("multisample gain in dB reaches the variant amplitude", "[importer][multisample]")
{
    auto p = buildMultisample(
        "gain", oneSample(R"(gain="-6.000")", R"(<key root="60"/><velocity/>)"), {{"A.wav", 1000}});
    Fixture f;
    f.load(p);

    CHECK(f.onlyZone().variantData.variants[0].amplitude ==
          Approx(scxt::import_support::dBToCubicAttenuation(-6.f)));
}

TEST_CASE("multisample gain is clamped to the parameter's upper bound", "[importer][multisample]")
{
    // SingleVariant::amplitude is described asCubicDecibelAttenuation with a
    // +12dB ceiling; a louder file must not write past it.
    auto p = buildMultisample("gain_hot",
                              oneSample(R"(gain="36.000")", R"(<key root="60"/><velocity/>)"),
                              {{"A.wav", 1000}});
    Fixture f;
    f.load(p);

    CHECK(f.onlyZone().variantData.variants[0].amplitude ==
          Approx(scxt::import_support::dBToCubicAttenuation(12.f)));
}

TEST_CASE("multisample velocity low defaults to 1", "[importer][multisample]")
{
    auto p = buildMultisample(
        "veldefault", oneSample("", R"(<key root="60"/><velocity high="90"/>)"), {{"A.wav", 1000}});
    Fixture f;
    f.load(p);

    auto &z = f.onlyZone();
    CHECK(z.mapping.velocityRange.velStart == 1);
    CHECK(z.mapping.velocityRange.velEnd == 90);
}

TEST_CASE("multisample accepts PreSonus boolean key tracking", "[importer][multisample]")
{
    // PreSonus writes track="true"/"false" where the spec says a double.
    auto p = buildMultisample("track_bool",
                              oneSample("", R"(<key root="60" track="false"/><velocity/>)"),
                              {{"A.wav", 1000}});
    Fixture f;
    f.load(p);

    CHECK(f.onlyZone().mapping.tracking == Approx(0.f));
}

TEST_CASE("multisample reads tune from the sample element", "[importer][multisample]")
{
    // PreSonus puts tune on <sample>; the spec puts it on <key>.
    auto p = buildMultisample("tune_sample",
                              oneSample(R"(tune="3.0")", R"(<key root="60"/><velocity/>)"),
                              {{"A.wav", 1000}});
    Fixture f;
    f.load(p);

    CHECK(f.onlyZone().mapping.pitchOffset == Approx(3.f));
}
