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
 * Covers the RIFF-WAV hardening: the EndStack/chunk-size clamp, the bounded
 * RIFFReadChunk size, the bounded INAM copy, and the cue/strc count caps.
 *
 * Also covers AIFF: the COMM frame count clamped to the SSND payload, and the
 * capped/zero-initialized MARK marker arrays.
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
    // a LIST:INFO chunk whose declared size dwarfs the file. Without the
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
    // INAM declares 10 bytes with no NUL and sits at the very end of the
    // buffer; the copy must stay within the chunk and NUL-terminate name[].
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
    // a 'cue ' chunk that claims 256M cue points but carries one. The count
    // must be capped to what the chunk can hold, not allocated wholesale.
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
    // count says 5, chunk header claims 5 records, but only 2 are present
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
    // 8 * WaveDataSize overflowed int32 above 256MB, yielding a negative
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

// ---------------------------------------------------------------------------
// AIFF (big-endian) builders + cases. parse_aiff reads everything big-endian.
// ---------------------------------------------------------------------------
namespace
{
void putU16BE(std::vector<uint8_t> &b, uint16_t v)
{
    b.push_back((v >> 8) & 0xFF);
    b.push_back(v & 0xFF);
}
void putU32BE(std::vector<uint8_t> &b, uint32_t v)
{
    b.push_back((v >> 24) & 0xFF);
    b.push_back((v >> 16) & 0xFF);
    b.push_back((v >> 8) & 0xFF);
    b.push_back(v & 0xFF);
}

// 80-bit IEEE-extended encoding of 44100.0, what ConvertFromIeeeExtended decodes.
const uint8_t kRate44100[10] = {0x40, 0x0E, 0xAC, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// COMM chunk: numChannels, numSampleFrames, sampleSize(bits), 10-byte rate.
void putAiffCOMM(std::vector<uint8_t> &b, uint16_t ch, uint32_t nframes, uint16_t bits)
{
    putTag(b, "COMM");
    putU32BE(b, 18); // 2 + 4 + 2 + 10
    putU16BE(b, ch);
    putU32BE(b, nframes);
    putU16BE(b, bits);
    b.insert(b.end(), kRate44100, kRate44100 + 10);
}

// SSND chunk: 4-byte offset, 4-byte blockSize, `offset` pad bytes, then audio.
void putAiffSSND(std::vector<uint8_t> &b, uint32_t offset, const std::vector<uint8_t> &audio)
{
    putTag(b, "SSND");
    putU32BE(b, (uint32_t)(8 + offset + audio.size()));
    putU32BE(b, offset);
    putU32BE(b, 0); // blockSize
    for (uint32_t i = 0; i < offset; ++i)
        b.push_back(0);
    b.insert(b.end(), audio.begin(), audio.end());
}

// Wrap chunks in the outer FORM/AIFF header (FORM size is big-endian).
std::vector<uint8_t> assembleAiff(const std::vector<uint8_t> &chunks)
{
    std::vector<uint8_t> f;
    putTag(f, "FORM");
    putU32BE(f, (uint32_t)(4 + chunks.size())); // "AIFF" + chunks
    putTag(f, "AIFF");
    f.insert(f.end(), chunks.begin(), chunks.end());
    return f;
}

std::vector<uint8_t> rampAudio(size_t bytes)
{
    std::vector<uint8_t> a(bytes);
    for (size_t i = 0; i < bytes; ++i)
        a[i] = (uint8_t)(i & 0xFF);
    return a;
}
} // namespace

TEST_CASE("parse_aiff: valid mono 16-bit baseline parses", "[sample][fuzz][aiff]")
{
    std::vector<uint8_t> chunks;
    putAiffCOMM(chunks, 1, 32, 16);
    putAiffSSND(chunks, 0, rampAudio(32 * 2));
    auto f = assembleAiff(chunks);

    sample::Sample s;
    REQUIRE(s.parse_aiff(f.data(), f.size()));
    CHECK(s.channels == 1);
    CHECK(s.sample_rate == 44100);
    CHECK(s.getSampleLength() == 32);
}

TEST_CASE("parse_aiff: COMM frame count exceeding SSND payload is clamped (mono)",
          "[sample][fuzz][aiff]")
{
    // COMM claims 100000 frames but SSND carries 16; the frame count must be
    // clamped to the payload or load_data_i16BE reads past the buffer (ASan).
    std::vector<uint8_t> chunks;
    putAiffCOMM(chunks, 1, 100000, 16);
    putAiffSSND(chunks, 0, rampAudio(16 * 2));
    auto f = assembleAiff(chunks);

    sample::Sample s;
    REQUIRE(s.parse_aiff(f.data(), f.size()));
    CHECK(s.getSampleLength() <= 16);
}

TEST_CASE("parse_aiff: COMM frame count exceeding SSND payload is clamped (stereo)",
          "[sample][fuzz][aiff]")
{
    // Same OOB for the stereo path: stride 4, both channels read nframes apart.
    std::vector<uint8_t> chunks;
    putAiffCOMM(chunks, 2, 100000, 16);
    putAiffSSND(chunks, 0, rampAudio(8 * 4));
    auto f = assembleAiff(chunks);

    sample::Sample s;
    REQUIRE(s.parse_aiff(f.data(), f.size()));
    CHECK(s.channels == 2);
    CHECK(s.getSampleLength() <= 8);
}

TEST_CASE("parse_aiff: 24-bit COMM frame count exceeding SSND payload is clamped",
          "[sample][fuzz][aiff]")
{
    // Exercises the i24 stride-3 path (bytesPerFrame derived from bitdepth/8).
    std::vector<uint8_t> chunks;
    putAiffCOMM(chunks, 1, 100000, 24);
    putAiffSSND(chunks, 0, rampAudio(10 * 3));
    auto f = assembleAiff(chunks);

    sample::Sample s;
    REQUIRE(s.parse_aiff(f.data(), f.size()));
    CHECK(s.getSampleLength() <= 10);
}

TEST_CASE("parse_aiff: huge MARK count does not over-allocate or read past the chunk",
          "[sample][fuzz][aiff]")
{
    // MARK claims 65535 markers but the chunk carries one. The count is
    // capped to what the chunk can hold; the loop bails on the short read.
    std::vector<uint8_t> chunks;
    putAiffCOMM(chunks, 1, 16, 16);
    putAiffSSND(chunks, 0, rampAudio(16 * 2));

    std::vector<uint8_t> mark;
    putU16BE(mark, 0xFFFF); // lie: 65535 markers
    putU16BE(mark, 0);      // marker id
    putU32BE(mark, 4);      // pos
    mark.push_back(0);      // pstring length 0
    mark.push_back(0);      // pad to even
    putTag(chunks, "MARK");
    putU32BE(chunks, (uint32_t)mark.size());
    chunks.insert(chunks.end(), mark.begin(), mark.end());

    auto f = assembleAiff(chunks);
    sample::Sample s;
    s.parse_aiff(f.data(), f.size()); // success/failure both fine; must not OOB
    SUCCEED();
}

TEST_CASE("parse_aiff: INST loop referencing an absent marker reads a defined value",
          "[sample][fuzz][aiff]")
{
    // zero-initialized marker arrays mean an INST sustain loop pointing at
    // a marker id that was never written resolves to 0 rather than stale heap.
    std::vector<uint8_t> chunks;
    putAiffCOMM(chunks, 1, 16, 16);
    putAiffSSND(chunks, 0, rampAudio(16 * 2));

    // MARK: declare 4 markers but only supply id 0.
    std::vector<uint8_t> mark;
    putU16BE(mark, 4);
    putU16BE(mark, 0); // id 0 only
    putU32BE(mark, 4); // pos
    mark.push_back(0);
    mark.push_back(0);
    putTag(chunks, "MARK");
    putU32BE(chunks, (uint32_t)mark.size());
    chunks.insert(chunks.end(), mark.begin(), mark.end());

    // INST (20 bytes): sustainLoop playmode=1, markers 2 and 3 (absent).
    std::vector<uint8_t> inst;
    for (int i = 0; i < 6; ++i)
        inst.push_back(0); // baseNote..highvelocity
    putU16BE(inst, 0);     // gain
    putU16BE(inst, 1);     // sustainLoop.playmode (looping)
    putU16BE(inst, 2);     // sustainLoop.marker_start (absent)
    putU16BE(inst, 3);     // sustainLoop.marker_end   (absent)
    putU16BE(inst, 0);     // releaseLoop.playmode
    putU16BE(inst, 0);     // releaseLoop.marker_start
    putU16BE(inst, 0);     // releaseLoop.marker_end
    putTag(chunks, "INST");
    putU32BE(chunks, (uint32_t)inst.size());
    chunks.insert(chunks.end(), inst.begin(), inst.end());

    auto f = assembleAiff(chunks);
    sample::Sample s;
    REQUIRE(s.parse_aiff(f.data(), f.size()));
    CHECK(s.meta.loop_start == 0);
    CHECK(s.meta.loop_end == 1); // absent marker → 0, code stores +1
}

TEST_CASE("parse_aiff: survives truncation at every prefix length", "[sample][fuzz][aiff]")
{
    // Full AIFF (COMM + SSND + MARK + INST) parsed at every prefix length. Under
    // ASan this catches an OOB read anywhere in the chunk walk for any cut point.
    std::vector<uint8_t> chunks;
    putAiffCOMM(chunks, 2, 24, 16);
    putAiffSSND(chunks, 4, rampAudio(24 * 4));

    std::vector<uint8_t> mark;
    putU16BE(mark, 2);
    putU16BE(mark, 0);
    putU32BE(mark, 0);
    mark.push_back(0);
    mark.push_back(0);
    putU16BE(mark, 1);
    putU32BE(mark, 12);
    mark.push_back(0);
    mark.push_back(0);
    putTag(chunks, "MARK");
    putU32BE(chunks, (uint32_t)mark.size());
    chunks.insert(chunks.end(), mark.begin(), mark.end());

    std::vector<uint8_t> inst;
    for (int i = 0; i < 6; ++i)
        inst.push_back(0);
    putU16BE(inst, 0);
    putU16BE(inst, 1);
    putU16BE(inst, 0);
    putU16BE(inst, 1);
    putU16BE(inst, 0);
    putU16BE(inst, 0);
    putU16BE(inst, 0);
    putTag(chunks, "INST");
    putU32BE(chunks, (uint32_t)inst.size());
    chunks.insert(chunks.end(), inst.begin(), inst.end());

    auto f = assembleAiff(chunks);
    for (size_t n = 1; n <= f.size(); ++n)
    {
        sample::Sample s;
        s.parse_aiff(f.data(), n);
    }
    SUCCEED();
}
