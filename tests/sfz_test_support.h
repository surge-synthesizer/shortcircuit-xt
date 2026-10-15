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

#ifndef SCXT_TESTS_SFZ_TEST_SUPPORT_H
#define SCXT_TESTS_SFZ_TEST_SUPPORT_H

#include "catch2/catch2.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "console_harness.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "engine/zone.h"

/*
 * Synthesized SFZ fixtures - an .sfz is text plus WAVs, so a case costs a few lines and the
 * sample lengths double as variant identity.
 */
namespace scxt::sfztest
{
namespace fs = std::filesystem;

inline void putU16(std::vector<uint8_t> &b, uint16_t v)
{
    b.push_back(v & 0xFF);
    b.push_back((v >> 8) & 0xFF);
}
inline void putU32(std::vector<uint8_t> &b, uint32_t v)
{
    b.push_back(v & 0xFF);
    b.push_back((v >> 8) & 0xFF);
    b.push_back((v >> 16) & 0xFF);
    b.push_back((v >> 24) & 0xFF);
}
inline void putTag(std::vector<uint8_t> &b, const char *t) { b.insert(b.end(), t, t + 4); }

// what a smpl chunk carries
struct SmplSpec
{
    uint32_t rootKey{60};
    std::optional<std::pair<uint32_t, uint32_t>> loop;
    uint32_t pitchFraction{0};
};

// what an inst chunk carries
struct InstSpec
{
    int8_t rootKey{60};
    int8_t fineTune{0};
    int8_t keyLow{0}, keyHigh{127};
    int8_t velLow{1}, velHigh{127};
};

// Mono 16-bit PCM at 48k, nframes long.
inline void writeWav(const fs::path &p, uint32_t nframes, std::optional<SmplSpec> smpl = {},
                     std::optional<InstSpec> inst = {})
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

    if (smpl)
    {
        auto nLoops = smpl->loop ? 1u : 0u;
        putTag(body, "smpl");
        putU32(body, 36 + 24 * nLoops);
        putU32(body, 0); // manufacturer
        putU32(body, 0); // product
        putU32(body, 1000000000 / 48000);
        putU32(body, smpl->rootKey);
        putU32(body, smpl->pitchFraction);
        putU32(body, 0); // smpte format
        putU32(body, 0); // smpte offset
        putU32(body, nLoops);
        putU32(body, 0); // sampler data
        if (smpl->loop)
        {
            putU32(body, 0); // cue id
            putU32(body, 0); // forward
            putU32(body, smpl->loop->first);
            putU32(body, smpl->loop->second);
            putU32(body, 0); // fraction
            putU32(body, 0); // play count
        }
    }

    if (inst)
    {
        putTag(body, "inst");
        putU32(body, 7);
        for (auto b : {inst->rootKey, inst->fineTune, (int8_t)0, inst->keyLow, inst->keyHigh,
                       inst->velLow, inst->velHigh})
            body.push_back((uint8_t)b);
        body.push_back(0); // pad to even
    }

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
    std::optional<SmplSpec> smpl{};
    std::optional<InstSpec> inst{};
};

inline fs::path buildSfz(const std::string &stem, const std::string &sfz,
                         const std::vector<WavSpec> &wavs)
{
    auto dir = fs::temp_directory_path() / "scxt-sfz-tests" / stem;
    fs::remove_all(dir);
    fs::create_directories(dir);
    for (const auto &w : wavs)
        writeWav(dir / w.name, w.frames, w.smpl, w.inst);

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
        th.sync();
    }

    scxt::engine::Part &part0() { return *th.engine->getPatch()->getPart(0); }

    void load(const fs::path &p)
    {
        th.sendToSerialization(scxt::messaging::client::AddSample(p.u8string()));
        th.sync();
    }

    scxt::engine::Zone &onlyZone()
    {
        auto &groups = part0().getGroups();
        REQUIRE(groups.size() == 1);
        REQUIRE(groups[0]->getZones().size() == 1);
        return *groups[0]->getZones()[0];
    }
};

inline int activeVariants(const scxt::engine::Zone &z)
{
    int n{0};
    for (const auto &v : z.variantData.variants)
        if (v.active)
            ++n;
    return n;
}
} // namespace scxt::sfztest

#endif
