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
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"

/*
 * SFZ lorand/hirand: a note-on draws one uniform number and only the regions
 * whose range covers it sound. Libraries use it as a round robin, so a stack of
 * same-geometry regions imports as one zone of randomly played variants.
 *
 * Fixtures are synthesized - an .sfz is text plus WAVs, so a case costs a few
 * lines and the sample lengths double as variant identity.
 */

namespace cmsg = scxt::messaging::client;
namespace fs = std::filesystem;

namespace
{

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
void putTag(std::vector<uint8_t> &b, const char *t) { b.insert(b.end(), t, t + 4); }

// Mono 16-bit PCM at 48k, nframes long.
void writeWav(const fs::path &p, uint32_t nframes)
{
    std::vector<uint8_t> body;
    putTag(body, "WAVE");

    putTag(body, "fmt ");
    putU32(body, 16);
    putU16(body, 1); // PCM
    putU16(body, 1); // mono
    putU32(body, 48000);
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

    std::vector<uint8_t> riff;
    putTag(riff, "RIFF");
    putU32(riff, (uint32_t)body.size());
    riff.insert(riff.end(), body.begin(), body.end());

    std::ofstream os(p, std::ios::binary);
    os.write((const char *)riff.data(), riff.size());
}

struct WavSpec
{
    std::string name;
    uint32_t frames;
};

fs::path buildSfz(const std::string &stem, const std::string &sfz, const std::vector<WavSpec> &wavs)
{
    auto dir = fs::temp_directory_path() / "scxt-sfz-rand-tests" / stem;
    fs::remove_all(dir);
    fs::create_directories(dir);
    for (const auto &w : wavs)
        writeWav(dir / w.name, w.frames);

    auto p = dir / (stem + ".sfz");
    std::ofstream os(p, std::ios::binary);
    os << sfz;
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

    scxt::engine::Zone &onlyZone()
    {
        auto &groups = part0().getGroups();
        REQUIRE(groups.size() == 1);
        REQUIRE(groups[0]->getZones().size() == 1);
        return *groups[0]->getZones()[0];
    }
};

int activeVariants(const scxt::engine::Zone &z)
{
    int n{0};
    for (const auto &v : z.variantData.variants)
        if (v.active)
            ++n;
    return n;
}

// Four equal buckets tiling [0,1), the shape every library that uses these
// opcodes writes.
const std::string quartet = R"(
<group>
lokey=60 hikey=60 pitch_keycenter=60

<region>
sample=a.wav
hirand=0.25

<region>
sample=b.wav
lorand=0.25
hirand=0.5

<region>
sample=c.wav
lorand=0.5
hirand=0.75

<region>
sample=d.wav
lorand=0.75
)";

const std::vector<WavSpec> quartetWavs{
    {"a.wav", 1000}, {"b.wav", 2000}, {"c.wav", 3000}, {"d.wav", 4000}};

} // namespace

TEST_CASE("SFZ lorand stack folds into one zone", "[importer][sfz]")
{
    auto p = buildSfz("quartet", quartet, quartetWavs);

    Fixture f;
    f.load(p);

    auto &z = f.onlyZone();
    CHECK(activeVariants(z) == 4);
    CHECK_FALSE(z.variantData.variants[4].active);
}

TEST_CASE("SFZ lorand stack plays its variants at random", "[importer][sfz]")
{
    // the opcode is a dice roll per note, repeats included, so nothing smarter
    // than true random is faithful to the file
    auto p = buildSfz("quartet_mode", quartet, quartetWavs);

    Fixture f;
    f.load(p);

    CHECK(f.onlyZone().variantData.variantPlaybackMode ==
          scxt::engine::Zone::VariantPlaybackMode::TRUE_RANDOM);
}

TEST_CASE("SFZ lorand variants keep file order", "[importer][sfz]")
{
    // the ranges could sort them, but the file's order is the author's order
    auto p = buildSfz("quartet_order", quartet, quartetWavs);

    Fixture f;
    f.load(p);

    auto &z = f.onlyZone();
    CHECK(z.variantData.variants[0].endSample == 1000);
    CHECK(z.variantData.variants[1].endSample == 2000);
    CHECK(z.variantData.variants[2].endSample == 3000);
    CHECK(z.variantData.variants[3].endSample == 4000);
}

TEST_CASE("SFZ lorand declared out of order still lands in file order", "[importer][sfz]")
{
    const std::string sfz = R"(
<group>
lokey=60 hikey=60 pitch_keycenter=60

<region>
sample=c.wav
lorand=0.5

<region>
sample=a.wav
hirand=0.5
)";
    auto p = buildSfz("descending", sfz, {{"a.wav", 1000}, {"c.wav", 3000}});

    Fixture f;
    f.load(p);

    auto &z = f.onlyZone();
    REQUIRE(activeVariants(z) == 2);
    CHECK(z.variantData.variants[0].endSample == 3000);
    CHECK(z.variantData.variants[1].endSample == 1000);
}

TEST_CASE("SFZ regions covering the whole rand range are not folded", "[importer][sfz]")
{
    // both always sound, so they are a layer, not an alternation
    const std::string sfz = R"(
<group>
lokey=60 hikey=60 pitch_keycenter=60

<region>
sample=a.wav
lorand=0
hirand=1

<region>
sample=b.wav
lorand=0
hirand=1
)";
    auto p = buildSfz("full_range", sfz, {{"a.wav", 1000}, {"b.wav", 2000}});

    Fixture f;
    f.load(p);

    auto &groups = f.part0().getGroups();
    REQUIRE(groups.size() == 1);
    CHECK(groups[0]->getZones().size() == 2);
}

TEST_CASE("SFZ lorand stack folds across a stray root key", "[importer][sfz]")
{
    // growlybass ships rr5 of several stacks a semitone off its siblings; the
    // ranges are what a voice lands on, so the odd root is a typo to absorb
    // rather than a reason to leave a zone always sounding
    const std::string sfz = R"(
<group>
lokey=47 hikey=49

<region>
sample=a.wav
pitch_keycenter=48
hirand=0.5

<region>
sample=b.wav
pitch_keycenter=47
lorand=0.5
)";
    auto p = buildSfz("stray_root", sfz, {{"a.wav", 1000}, {"b.wav", 2000}});

    Fixture f;
    f.load(p);

    auto &z = f.onlyZone();
    REQUIRE(activeVariants(z) == 2);
    CHECK(z.mapping.rootKey == 48);
    // a semitone of root shifted onto the variant keeps it sounding as written
    CHECK(z.variantData.variants[0].pitchOffset == Approx(0.f));
    CHECK(z.variantData.variants[1].pitchOffset == Approx(1.f));
}

TEST_CASE("SFZ lorand stack carries per region transpose onto its variants", "[importer][sfz]")
{
    // the sfz1/sfz2 conformance fixture for lorand: one sample at four
    // transpositions, picked at random. transpose lands on the zone, so folding
    // has to move the difference onto the variant or all four sound alike
    const std::string sfz = R"(
<group>
sample=a.wav
lokey=60 hikey=60 pitch_keycenter=60

<region> transpose=0 lorand=0    hirand=0.25
<region> transpose=3 lorand=0.25 hirand=0.5
<region> transpose=6 lorand=0.5  hirand=0.75
<region> transpose=8 lorand=0.75 hirand=1
)";
    auto p = buildSfz("transposed", sfz, {{"a.wav", 1000}});

    Fixture f;
    f.load(p);

    auto &z = f.onlyZone();
    REQUIRE(activeVariants(z) == 4);
    CHECK(z.variantData.variants[0].pitchOffset == Approx(0.f));
    CHECK(z.variantData.variants[1].pitchOffset == Approx(3.f));
    CHECK(z.variantData.variants[2].pitchOffset == Approx(6.f));
    CHECK(z.variantData.variants[3].pitchOffset == Approx(8.f));
}

TEST_CASE("SFZ velocity layers of rand stacks fold one zone each", "[importer][sfz]")
{
    // the common growlybass shape: one group holding a rand stack per velocity
    // layer. Each layer is its own zone and none of it is a mapping problem
    std::string sfz = "<group>\nlokey=60 hikey=60 pitch_keycenter=60\n";
    std::vector<WavSpec> wavs;
    const int vel[3][2] = {{1, 60}, {61, 95}, {96, 127}};
    for (int l = 0; l < 3; ++l)
    {
        for (int r = 0; r < 2; ++r)
        {
            auto name = "l" + std::to_string(l) + "r" + std::to_string(r) + ".wav";
            sfz += "\n<region>\nsample=" + name + "\nlovel=" + std::to_string(vel[l][0]) +
                   "\nhivel=" + std::to_string(vel[l][1]) + "\n" +
                   (r == 0 ? "hirand=0.5\n" : "lorand=0.5\n");
            wavs.push_back({name, (uint32_t)(1000 * (l * 2 + r + 1))});
        }
    }
    auto p = buildSfz("vel_layers", sfz, wavs);

    Fixture f;
    f.th.editor->clearWarnings();
    f.load(p);

    auto &groups = f.part0().getGroups();
    REQUIRE(groups.size() == 1);
    auto &zones = groups[0]->getZones();
    REQUIRE(zones.size() == 3);
    for (const auto &z : zones)
        CHECK(activeVariants(*z) == 2);

    CHECK(f.th.editor->readWarnings().empty());
}

TEST_CASE("SFZ rand region with no partners is reported", "[importer][sfz]")
{
    // it was meant to sound on its slice of the dice and will now sound on
    // every note, which is the case worth telling the user about
    const std::string sfz = R"(
<group>
lokey=60 hikey=60 pitch_keycenter=60

<region>
sample=a.wav
hirand=0.5

<region>
sample=b.wav
lorand=0.5

<region>
sample=c.wav
lokey=72 hikey=72
lorand=0.5
)";
    auto p = buildSfz("stranded", sfz, {{"a.wav", 1000}, {"b.wav", 2000}, {"c.wav", 3000}});

    Fixture f;
    f.th.editor->clearWarnings();
    f.load(p);

    CHECK_FALSE(f.th.editor->readWarnings().empty());
}

TEST_CASE("SFZ lorand regions of different geometry stay separate", "[importer][sfz]")
{
    // the pair on 60 folds; the odd key out cannot join them
    const std::string sfz = R"(
<group>
pitch_keycenter=60

<region>
sample=a.wav
lokey=60 hikey=60
hirand=0.5

<region>
sample=b.wav
lokey=60 hikey=60
lorand=0.5

<region>
sample=c.wav
lokey=62 hikey=62
lorand=0.5
)";
    auto p = buildSfz("mixed_geometry", sfz, {{"a.wav", 1000}, {"b.wav", 2000}, {"c.wav", 3000}});

    Fixture f;
    f.load(p);

    auto &groups = f.part0().getGroups();
    REQUIRE(groups.size() == 1);
    auto &zones = groups[0]->getZones();
    REQUIRE(zones.size() == 2);
    CHECK(activeVariants(*zones[0]) == 2);
    CHECK(activeVariants(*zones[1]) == 1);
}

TEST_CASE("SFZ uneven rand ranges still fold", "[importer][sfz]")
{
    // a weighted dice: scxt variants have no weight, so it imports evenly
    const std::string sfz = R"(
<group>
lokey=60 hikey=60 pitch_keycenter=60

<region>
sample=a.wav
hirand=0.5

<region>
sample=b.wav
lorand=0.5
hirand=0.75

<region>
sample=c.wav
lorand=0.75
)";
    auto p = buildSfz("uneven", sfz, {{"a.wav", 1000}, {"b.wav", 2000}, {"c.wav", 3000}});

    Fixture f;
    f.load(p);

    CHECK(activeVariants(f.onlyZone()) == 3);
}

TEST_CASE("SFZ rand regions leave plain regions alone", "[importer][sfz]")
{
    const std::string sfz = R"(
<group>
lokey=60 hikey=60 pitch_keycenter=60

<region>
sample=plain.wav
lokey=48 hikey=48

<region>
sample=a.wav
hirand=0.5

<region>
sample=b.wav
lorand=0.5
)";
    auto p = buildSfz("with_plain", sfz, {{"plain.wav", 500}, {"a.wav", 1000}, {"b.wav", 2000}});

    Fixture f;
    f.load(p);

    auto &groups = f.part0().getGroups();
    REQUIRE(groups.size() == 1);
    auto &zones = groups[0]->getZones();
    REQUIRE(zones.size() == 2);
    CHECK(activeVariants(*zones[0]) == 1);
    CHECK(zones[0]->mapping.keyboardRange.keyStart == 48);
    CHECK(activeVariants(*zones[1]) == 2);
}

TEST_CASE("SFZ rand stack past the variant cap spills into another zone", "[importer][sfz]")
{
    const int n = scxt::maxVariantsPerZone + 1;
    std::string sfz = "<group>\nlokey=60 hikey=60 pitch_keycenter=60\n";
    std::vector<WavSpec> wavs;
    for (int i = 0; i < n; ++i)
    {
        auto name = "s" + std::to_string(i) + ".wav";
        sfz += "\n<region>\nsample=" + name + "\nlorand=" + std::to_string((float)i / n) +
               "\nhirand=" + std::to_string((float)(i + 1) / n) + "\n";
        wavs.push_back({name, (uint32_t)(100 * (i + 1))});
    }
    auto p = buildSfz("overflow", sfz, wavs);

    Fixture f;
    f.load(p);

    auto &groups = f.part0().getGroups();
    REQUIRE(groups.size() == 1);
    auto &zones = groups[0]->getZones();
    REQUIRE(zones.size() == 2);
    CHECK(activeVariants(*zones[0]) == scxt::maxVariantsPerZone);
    CHECK(activeVariants(*zones[1]) == 1);
}

TEST_CASE("SFZ rand opcodes are not reported as unused", "[importer][sfz]")
{
    auto p = buildSfz("unused", quartet, quartetWavs);

    Fixture f;
    f.th.editor->clearUnusedItems();
    f.load(p);

    for (const auto &[format, key, value] : f.th.editor->readUnusedItems())
    {
        INFO("unused " << format << " " << key);
        CHECK(key != "lorand");
        CHECK(key != "hirand");
    }
}

TEST_CASE("SFZ rand stacks fold per group", "[importer][sfz]")
{
    const std::string sfz = R"(
<group>
lokey=60 hikey=60 pitch_keycenter=60

<region>
sample=a.wav
hirand=0.5

<region>
sample=b.wav
lorand=0.5

<group>
lokey=60 hikey=60 pitch_keycenter=60

<region>
sample=c.wav
hirand=0.5

<region>
sample=d.wav
lorand=0.5
)";
    auto p = buildSfz("two_groups", sfz, quartetWavs);

    Fixture f;
    f.load(p);

    auto &groups = f.part0().getGroups();
    REQUIRE(groups.size() == 2);
    REQUIRE(groups[0]->getZones().size() == 1);
    REQUIRE(groups[1]->getZones().size() == 1);
    CHECK(activeVariants(*groups[0]->getZones()[0]) == 2);
    CHECK(activeVariants(*groups[1]->getZones()[0]) == 2);
}
