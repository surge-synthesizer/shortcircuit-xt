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
#include <cstdint>
#include <vector>

#if SCXT_USE_FLAC
#include "FLAC/stream_encoder.h"
#endif

// 8 and 12 bit load to I16, everything above 16 bit loads to F32, all full scale
namespace bitdepthtest
{
using scxt::sample::Sample;

// -full, -half, zero, +half at the source depth
std::vector<int32_t> testValues(int bits)
{
    int64_t full = 1LL << (bits - 1);
    return {(int32_t)-full, (int32_t)(-full / 2), 0, (int32_t)(full / 2)};
}

void checkI16(Sample &s, int bits, int channels)
{
    REQUIRE(s.bitDepth == Sample::BD_I16);
    REQUIRE(s.channels == channels);
    auto v = testValues(bits);
    REQUIRE(s.getSampleLength() == v.size());
    for (int c = 0; c < channels; ++c)
    {
        auto *d = s.GetSamplePtrI16(c);
        REQUIRE(d);
        for (size_t i = 0; i < v.size(); ++i)
        {
            INFO("bits=" << bits << " ch=" << c << " i=" << i);
            CHECK(d[i] == (int16_t)(v[i] * (1 << (16 - bits))));
        }
    }
}

void checkF32(Sample &s, int bits, int channels)
{
    REQUIRE(s.bitDepth == Sample::BD_F32);
    REQUIRE(s.channels == channels);
    const float expected[4] = {-1.f, -0.5f, 0.f, 0.5f};
    REQUIRE(s.getSampleLength() == 4);
    for (int c = 0; c < channels; ++c)
    {
        auto *d = s.GetSamplePtrF32(c);
        REQUIRE(d);
        for (size_t i = 0; i < 4; ++i)
        {
            INFO("bits=" << bits << " ch=" << c << " i=" << i);
            CHECK(d[i] == Approx(expected[i]).margin(1e-6));
        }
    }
}

void check(Sample &s, int bits, int channels)
{
    if (bits <= 16)
        checkI16(s, bits, channels);
    else
        checkF32(s, bits, channels);
}

void put(std::vector<uint8_t> &b, uint32_t v, int bytes, bool bigEndian)
{
    for (int i = 0; i < bytes; ++i)
    {
        int shift = bigEndian ? 8 * (bytes - 1 - i) : 8 * i;
        b.push_back((v >> shift) & 0xFF);
    }
}
void putTag(std::vector<uint8_t> &b, const char *t) { b.insert(b.end(), t, t + 4); }

// interleaved frames, left-justified in whole bytes. 8 bit wav is unsigned, aiff signed
std::vector<uint8_t> pcmData(int bits, int channels, bool bigEndian, bool unsigned8)
{
    std::vector<uint8_t> d;
    int bytes = (bits + 7) / 8;
    for (auto v : testValues(bits))
        for (int c = 0; c < channels; ++c)
        {
            uint32_t u = (uint32_t)v << (bytes * 8 - bits);
            if (bytes == 1 && unsigned8)
                u = (uint32_t)(v + 128);
            put(d, u, bytes, bigEndian);
        }
    return d;
}

std::vector<uint8_t> makeWav(int bits, int channels)
{
    auto data = pcmData(bits, channels, false, true);
    int blockAlign = channels * ((bits + 7) / 8);

    std::vector<uint8_t> body;
    putTag(body, "WAVE");
    putTag(body, "fmt ");
    put(body, 16, 4, false);
    put(body, 1, 2, false); // PCM
    put(body, channels, 2, false);
    put(body, 48000, 4, false);
    put(body, 48000 * blockAlign, 4, false);
    put(body, blockAlign, 2, false);
    put(body, bits, 2, false);
    putTag(body, "data");
    put(body, (uint32_t)data.size(), 4, false);
    body.insert(body.end(), data.begin(), data.end());

    std::vector<uint8_t> f;
    putTag(f, "RIFF");
    put(f, (uint32_t)body.size(), 4, false);
    f.insert(f.end(), body.begin(), body.end());
    return f;
}

std::vector<uint8_t> makeAiff(int bits, int channels)
{
    auto data = pcmData(bits, channels, true, false);
    // 80-bit IEEE-extended 44100
    const uint8_t rate[10] = {0x40, 0x0E, 0xAC, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    std::vector<uint8_t> chunks;
    putTag(chunks, "COMM");
    put(chunks, 18, 4, true);
    put(chunks, channels, 2, true);
    put(chunks, (uint32_t)testValues(bits).size(), 4, true);
    put(chunks, bits, 2, true);
    chunks.insert(chunks.end(), rate, rate + 10);
    putTag(chunks, "SSND");
    put(chunks, (uint32_t)(8 + data.size()), 4, true);
    put(chunks, 0, 4, true);
    put(chunks, 0, 4, true);
    chunks.insert(chunks.end(), data.begin(), data.end());

    std::vector<uint8_t> f;
    putTag(f, "FORM");
    put(f, (uint32_t)(4 + chunks.size()), 4, true);
    putTag(f, "AIFF");
    f.insert(f.end(), chunks.begin(), chunks.end());
    return f;
}

#if SCXT_USE_FLAC
FLAC__StreamEncoderWriteStatus flacWrite(const FLAC__StreamEncoder *, const FLAC__byte buffer[],
                                         size_t bytes, uint32_t, uint32_t, void *ud)
{
    auto *out = static_cast<std::vector<uint8_t> *>(ud);
    out->insert(out->end(), buffer, buffer + bytes);
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

std::vector<uint8_t> makeFlac(int bits, int channels)
{
    auto v = testValues(bits);
    std::vector<FLAC__int32> interleaved;
    for (auto x : v)
        for (int c = 0; c < channels; ++c)
            interleaved.push_back(x);

    std::vector<uint8_t> out;
    auto *enc = FLAC__stream_encoder_new();
    REQUIRE(enc);
    FLAC__stream_encoder_set_streamable_subset(enc, false);
    FLAC__stream_encoder_set_channels(enc, channels);
    FLAC__stream_encoder_set_bits_per_sample(enc, bits);
    FLAC__stream_encoder_set_sample_rate(enc, 48000);
    FLAC__stream_encoder_set_total_samples_estimate(enc, v.size());
    REQUIRE(FLAC__stream_encoder_init_stream(enc, flacWrite, nullptr, nullptr, nullptr, &out) ==
            FLAC__STREAM_ENCODER_INIT_STATUS_OK);
    REQUIRE(FLAC__stream_encoder_process_interleaved(enc, interleaved.data(), v.size()));
    REQUIRE(FLAC__stream_encoder_finish(enc));
    FLAC__stream_encoder_delete(enc);
    return out;
}
#endif
} // namespace bitdepthtest

TEST_CASE("WAV bit depths load at full scale", "[sample][wav][bitdepth]")
{
    for (int bits : {8, 12, 16, 20, 24, 32})
        for (int ch : {1, 2})
        {
            DYNAMIC_SECTION("bits=" << bits << " channels=" << ch)
            {
                auto f = bitdepthtest::makeWav(bits, ch);
                scxt::sample::Sample s;
                REQUIRE(s.parse_riff_wave(f.data(), f.size()));
                bitdepthtest::check(s, bits, ch);
            }
        }
}

TEST_CASE("AIFF bit depths load at full scale", "[sample][aiff][bitdepth]")
{
    for (int bits : {8, 12, 16, 20, 24, 32})
        for (int ch : {1, 2})
        {
            DYNAMIC_SECTION("bits=" << bits << " channels=" << ch)
            {
                auto f = bitdepthtest::makeAiff(bits, ch);
                scxt::sample::Sample s;
                REQUIRE(s.parse_aiff(f.data(), f.size()));
                bitdepthtest::check(s, bits, ch);
            }
        }
}

#if SCXT_USE_FLAC
TEST_CASE("FLAC bit depths load at full scale", "[sample][flac][bitdepth]")
{
    for (int bits : {8, 12, 16, 20, 24, 32})
        for (int ch : {1, 2})
        {
            DYNAMIC_SECTION("bits=" << bits << " channels=" << ch)
            {
                auto f = bitdepthtest::makeFlac(bits, ch);
                scxt::sample::Sample s;
                REQUIRE(s.parseFlac(f.data(), f.size()));
                bitdepthtest::check(s, bits, ch);
            }
        }
}
#endif
