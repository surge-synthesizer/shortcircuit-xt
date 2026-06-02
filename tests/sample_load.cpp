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
#include "sample/sample.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>

#ifndef SCXT_TEST_SOURCE_DIR
#define SCXT_TEST_SOURCE_DIR ""
#endif

using namespace scxt;
namespace fs = std::filesystem;

static fs::path samplePath(const std::string &relative)
{
    return fs::path(SCXT_TEST_SOURCE_DIR) / "resources" / "test_samples" / relative;
}

TEST_CASE("Load Opus sample", "[sample][opus]")
{
    // 聲音不好.opus is a libopus encode of the sibling .wav fixture. Opus always
    // decodes to 48kHz float.
    auto p = samplePath("\xE8\x81\xB2\xE9\x9F\xB3\xE4\xB8\x8D\xE5\xA5\xBD.opus");
    INFO("fixture=" << p.u8string());
    REQUIRE(fs::exists(p));

    sample::Sample s;
    REQUIRE(s.load(p));

    CHECK(s.type == sample::Sample::OPUS_FILE);
    CHECK(s.sample_loaded);
    CHECK(s.sample_rate == 48000);
    CHECK(s.bitDepth == sample::Sample::BD_F32);
    CHECK(s.channels == 2);
    CHECK(s.getSampleLength() > 0);

    // ~0.96s of audio at 48kHz
    CHECK(s.getSampleLength() > 40000);
    CHECK(s.getSampleLength() < 50000);

    // The decoded buffer should contain some non-zero signal.
    auto *d0 = s.GetSamplePtrF32(0);
    REQUIRE(d0 != nullptr);
    float peak = 0.f;
    for (size_t i = 0; i < s.getSampleLength(); ++i)
        peak = std::max(peak, std::fabs(d0[i]));
    CHECK(peak > 0.01f);
}

TEST_CASE("Opus parses from in-memory bytes", "[sample][opus]")
{
    // The embedded-in-monolith path decodes from a byte buffer via op_open_memory.
    auto p = samplePath("\xE8\x81\xB2\xE9\x9F\xB3\xE4\xB8\x8D\xE5\xA5\xBD.opus");
    REQUIRE(fs::exists(p));

    std::ifstream in(p, std::ios::binary);
    REQUIRE(in.good());
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                               std::istreambuf_iterator<char>());
    REQUIRE(!bytes.empty());

    sample::Sample s;
    REQUIRE(s.parseOpus(bytes.data(), bytes.size()));
    CHECK(s.sample_rate == 48000);
    CHECK(s.channels == 2);
    CHECK(s.getSampleLength() > 0);
}
