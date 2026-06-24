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
 * In-memory fuzz harness for the binary sample-file parsers. The point is to
 * feed malformed/truncated buffers straight into the parser entry points
 * (Sample::parse_riff_wave(void*, size_t)) and assert they fail gracefully
 * rather than crash. Build with -DSCXT_SANITIZE=ON to turn the "didn't
 * obviously crash" checks into real OOB/overflow assertions (ASan+UBSan).
 *
 * Targets the S1 / RIFF-WAV hardening batch: SMP-1 (EndStack clamp), SMP-2
 * (RIFFReadChunk DataSize), SMP-4 (INAM bounded copy), SMP-5 (cue/strc count).
 */

#include "catch2/catch2.hpp"
#include "sample/sample.h"
#include <cstdint>
#include <cstring>
#include <vector>

using namespace scxt;

namespace
{
// Little-endian appenders + ASCII fourcc, matching what RIFFMemFile reads.
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
void patchU32(std::vector<uint8_t> &b, size_t at, uint32_t v)
{
    b[at + 0] = v & 0xFF;
    b[at + 1] = (v >> 8) & 0xFF;
    b[at + 2] = (v >> 16) & 0xFF;
    b[at + 3] = (v >> 24) & 0xFF;
}

void putFmt(std::vector<uint8_t> &b, uint16_t fmt, uint16_t ch, uint32_t rate, uint16_t bits)
{
    putTag(b, "fmt ");
    putU32(b, 16);
    putU16(b, fmt);                       // wFormatTag (1 == PCM)
    putU16(b, ch);                        // nChannels
    putU32(b, rate);                      // nSamplesPerSec
    putU32(b, rate * ch * bits / 8);      // nAvgBytesPerSec
    putU16(b, (uint16_t)(ch * bits / 8)); // nBlockAlign
    putU16(b, bits);                      // wBitsPerSample
}

// The bytes after the 8-byte "RIFF"+size header: "WAVE" + fmt + data chunks.
// Tests append further chunks before assembleRiff().
std::vector<uint8_t> wavBody(uint32_t nframes = 16)
{
    std::vector<uint8_t> body;
    putTag(body, "WAVE");
    putFmt(body, 1, 2, 48000, 16);
    putTag(body, "data");
    uint32_t dataBytes = nframes * 2 /*ch*/ * 2 /*bytes16*/;
    putU32(body, dataBytes);
    for (uint32_t i = 0; i < dataBytes; ++i)
        body.push_back((uint8_t)(i & 0xFF));
    return body;
}

// Wrap a body in the outer RIFF header. sizeOverride < 0 => correct size.
std::vector<uint8_t> assembleRiff(const std::vector<uint8_t> &body, int64_t sizeOverride = -1)
{
    std::vector<uint8_t> f;
    putTag(f, "RIFF");
    putU32(f, sizeOverride >= 0 ? (uint32_t)sizeOverride : (uint32_t)body.size());
    f.insert(f.end(), body.begin(), body.end());
    return f;
}
} // namespace

TEST_CASE("parse_riff_wave: valid baseline parses", "[sample][fuzz][wav]")
{
    auto f = assembleRiff(wavBody(32));
    sample::Sample s;
    REQUIRE(s.parse_riff_wave(f.data(), f.size()));
    CHECK(s.channels == 2);
    CHECK(s.sample_rate == 48000);
    CHECK(s.getSampleLength() == 32);
}

TEST_CASE("parse_riff_wave: inflated LIST size cannot read past the buffer", "[sample][fuzz][wav]")
{
    // SMP-1: a LIST:INFO chunk whose declared size dwarfs the file. Without the
    // EndStack clamp the INFO scan walks off the end of the buffer (ASan trip).
    auto body = wavBody();
    putTag(body, "LIST");
    putU32(body, 0x7FFFFF00); // wildly larger than the buffer
    putTag(body, "INFO");
    putTag(body, "INAM");
    putU32(body, 8);
    const uint8_t nm[8] = {'h', 'e', 'l', 'l', 'o', '!', '!', 0};
    body.insert(body.end(), nm, nm + 8);

    auto f = assembleRiff(body); // outer RIFF size is honest; the LIST lies
    sample::Sample s;
    // Must terminate without reading out of bounds; success/failure both fine.
    s.parse_riff_wave(f.data(), f.size());
    SUCCEED();
}

TEST_CASE("parse_riff_wave: short un-terminated INAM is copied safely", "[sample][fuzz][wav]")
{
    // SMP-4 (+SMP-2): INAM declares 10 bytes with no NUL and sits at the very end
    // of the buffer. The old strncpy(name, ptr, 64) read 54 bytes past EOF.
    auto body = wavBody();
    putTag(body, "LIST");
    size_t listSizeAt = body.size();
    putU32(body, 0); // patched below
    putTag(body, "INFO");
    putTag(body, "INAM");
    putU32(body, 10);
    for (int i = 0; i < 10; ++i)
        body.push_back((uint8_t)('A' + i)); // no NUL terminator
    // LIST payload = "INFO" + INAM header(8) + 10 bytes
    patchU32(body, listSizeAt, (uint32_t)(4 + 8 + 10));

    auto f = assembleRiff(body);
    sample::Sample s;
    REQUIRE(s.parse_riff_wave(f.data(), f.size()));
    // name must be NUL-terminated within its 64-byte buffer
    CHECK(s.name[63] == 0);
    CHECK(strnlen(s.name, sizeof(s.name)) == 10);
}

TEST_CASE("parse_riff_wave: huge cue count does not over-allocate", "[sample][fuzz][wav]")
{
    // SMP-5: a 'cue ' chunk that claims 256M cue points but carries one. Old code
    // did new int[268435456] twice. New code caps to bytesRemaining().
    auto body = wavBody();
    putTag(body, "cue ");
    putU32(body, 4 + 24);     // chunk size: count(4) + one CuePoint(24)
    putU32(body, 0x10000000); // 268,435,456 cue points (lie)
    for (int i = 0; i < 24; ++i)
        body.push_back(0);

    auto f = assembleRiff(body);
    sample::Sample s;
    REQUIRE(s.parse_riff_wave(f.data(), f.size()));
    // clamped to what the chunk can hold (1) => below the >1 threshold => no slices
    CHECK(s.meta.n_slices == 0);
}

TEST_CASE("parse_riff_wave: truncated cue chunk leaves no garbage slices", "[sample][fuzz][wav]")
{
    // SMP-5: count says 5, chunk header claims 5 records, but only 2 are present
    // before EOF. Must not read uninitialized memory; n_slices reflects reality.
    auto body = wavBody();
    putTag(body, "cue ");
    putU32(body, 4 + 5 * 24); // header claims room for 5
    putU32(body, 5);          // count: 5
    for (int i = 0; i < 2 * 24; ++i)
        body.push_back((uint8_t)(i & 0xFF)); // only 2 CuePoints actually present

    auto f = assembleRiff(body);
    sample::Sample s;
    REQUIRE(s.parse_riff_wave(f.data(), f.size()));
    CHECK(s.meta.n_slices <= 2);
}

TEST_CASE("parse_riff_wave: survives truncation at every prefix length", "[sample][fuzz][wav]")
{
    // The core fuzz: a feature-rich WAV (data + smpl + cue + LIST:INFO) parsed at
    // every prefix length. Under ASan this catches OOB reads anywhere in the
    // descent/chunk-walk for any truncation point.
    auto body = wavBody();

    // smpl chunk (root note + one loop)
    putTag(body, "smpl");
    putU32(body, 36 + 24); // SamplerChunk(36) + one SampleLoop(24)
    for (int i = 0; i < 36; ++i)
        body.push_back(0);
    putU32(body, 0);  // SampleLoop.dwIdentifier
    putU32(body, 0);  // dwType
    putU32(body, 4);  // dwStart
    putU32(body, 12); // dwEnd
    putU32(body, 0);
    putU32(body, 0);
    // overwrite cSampleLoops (field index 7 of SamplerChunk) so the loop is read
    // SamplerChunk layout: 9 int32; cSampleLoops is the 8th (offset 28 in chunk data)
    // chunk data starts right after the 8-byte "smpl"+size header.
    {
        size_t smplDataStart = body.size() - (36 + 24);
        patchU32(body, smplDataStart + 28, 1); // cSampleLoops = 1
    }

    // cue chunk (2 points)
    putTag(body, "cue ");
    putU32(body, 4 + 2 * 24);
    putU32(body, 2);
    for (int i = 0; i < 2 * 24; ++i)
        body.push_back((uint8_t)(i & 0xFF));

    // LIST:INFO with an INAM
    putTag(body, "LIST");
    size_t listSizeAt = body.size();
    putU32(body, 0);
    putTag(body, "INFO");
    putTag(body, "INAM");
    putU32(body, 6);
    const uint8_t nm[6] = {'h', 'i', 't', 'h', 'r', 0};
    body.insert(body.end(), nm, nm + 6);
    patchU32(body, listSizeAt, (uint32_t)(4 + 8 + 6));

    auto f = assembleRiff(body);

    for (size_t n = 1; n <= f.size(); ++n)
    {
        sample::Sample s;
        // Any return value is acceptable; the requirement is "does not crash /
        // read out of bounds" for an arbitrarily truncated file.
        s.parse_riff_wave(f.data(), n);
    }
    SUCCEED();
}

TEST_CASE("parse_riff_wave: byte-flip fuzz of size fields does not crash", "[sample][fuzz][wav]")
{
    auto base = assembleRiff(wavBody(64));
    // Flip the high bit of every 4-aligned word (hits chunk-size fields among
    // others) and parse each mutant.
    for (size_t off = 0; off + 4 <= base.size(); off += 4)
    {
        auto m = base;
        m[off + 3] ^= 0x80;
        sample::Sample s;
        s.parse_riff_wave(m.data(), m.size());
    }
    SUCCEED();
}

TEST_CASE("parse_riff_wave: 256MB+ data chunk sample count does not overflow",
          "[.][sample][fuzz][wav][smp3]")
{
    // SMP-3: 8 * WaveDataSize overflowed int32 above 256MB, yielding a negative
    // sample count. Hidden by default ([.]) because it allocates ~256MB; run
    // explicitly with `scxt-test "[smp3]"`.
    const uint32_t dataBytes = 256u * 1024 * 1024 + 4096; // just over the int32 cliff
    std::vector<uint8_t> body;
    putTag(body, "WAVE");
    putFmt(body, 1, 1, 48000, 8); // 8-bit mono: 1 byte per sample
    putTag(body, "data");
    putU32(body, dataBytes);
    body.resize(body.size() + dataBytes, 0);

    auto f = assembleRiff(body, /*sizeOverride*/ (int64_t)body.size());
    sample::Sample s;
    REQUIRE(s.parse_riff_wave(f.data(), f.size()));
    CHECK(s.getSampleLength() == dataBytes); // 8 * bytes / 8 == bytes, no wrap
}
