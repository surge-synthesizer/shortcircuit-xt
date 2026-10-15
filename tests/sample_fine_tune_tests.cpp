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

#include <cmath>
#include <cstring>

#include "sfz_test_support.h"
#include "sample/sample.h"
#include "sample/loaders/riff_wave.h"
#include "messaging/client/client_messages.h"

#if SCXT_USE_FLAC
#include "FLAC/metadata.h"
#include "FLAC/stream_encoder.h"
#endif

using namespace scxt::sfztest;
namespace cmsg = scxt::messaging::client;

namespace
{
uint32_t fractionFor(double cents) { return (uint32_t)std::llround(cents / 100.0 * 4294967296.0); }

fs::path tuneDir(const std::string &stem)
{
    auto dir = fs::temp_directory_path() / "scxt-fine-tune-tests" / stem;
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

const scxt::sample::Sample &sampleOf(const scxt::engine::Zone &z)
{
    REQUIRE(z.samplePointers[0]);
    return *z.samplePointers[0];
}

struct SmplCase
{
    const char *name;
    uint32_t unity;
    double fractionCents;
    int root;
    float cents;
};

// exactly 50 stays on the written key, so the fold only starts above it
const SmplCase smplCases[] = {
    {"no fraction", 60, 0.0, 60, 0.f},
    {"25 cents sharp tunes down", 60, 25.0, 60, -25.f},
    {"E3 plus 90 is F3 tuned up 10", 52, 90.0, 53, 10.f},
    {"exactly 50", 60, 50.0, 60, -50.f},
    {"top key cannot fold", 127, 90.0, 127, -90.f},
};
} // namespace

TEST_CASE("smpl pitch fraction folds to the nearest key", "[sample][finetune]")
{
    for (const auto &c : smplCases)
    {
        DYNAMIC_SECTION(c.name)
        {
            scxt::sample::loaders::SamplerChunk chunk{};
            chunk.dwMIDIUnityNote = (int32_t)c.unity;
            chunk.dwMIDIPitchFraction = (int32_t)fractionFor(c.fractionCents);
            auto t = scxt::sample::loaders::smplTuning(chunk);
            CHECK(t.rootKey == c.root);
            CHECK(t.cents == Approx(c.cents).margin(1e-4));
        }
    }
}

TEST_CASE("WAV smpl fine tune lands in the dropped zone", "[sample][finetune]")
{
    for (const auto &c : smplCases)
    {
        DYNAMIC_SECTION(c.name)
        {
            auto p = tuneDir("smpl") / "s.wav";
            writeWav(p, 1000, SmplSpec{c.unity, {}, fractionFor(c.fractionCents)});

            Fixture f;
            f.load(p);
            auto &z = f.onlyZone();

            const auto &m = sampleOf(z).meta;
            CHECK(m.rootkey_present);
            CHECK((int)m.key_root == c.root);
            CHECK(m.detune == Approx(c.cents).margin(1e-4));

            CHECK(z.mapping.rootKey == c.root);
            CHECK(z.mapping.pitchOffset == Approx(c.cents / 100.f).margin(1e-6));
        }
    }
}

TEST_CASE("WAV smpl wins the root key over inst", "[sample][finetune]")
{
    auto p = tuneDir("smpl_inst") / "s.wav";
    writeWav(p, 1000, SmplSpec{52, {}, fractionFor(90.0)},
             InstSpec{.rootKey = 40,
                      .fineTune = 20,
                      .keyLow = 30,
                      .keyHigh = 70,
                      .velLow = 10,
                      .velHigh = 100});

    Fixture f;
    f.load(p);
    auto &z = f.onlyZone();

    const auto &m = sampleOf(z).meta;
    CHECK((int)m.key_root == 53);
    CHECK(m.detune == Approx(10.f).margin(1e-4));

    CHECK(z.mapping.rootKey == 53);
    CHECK(z.mapping.pitchOffset == Approx(0.1f).margin(1e-6));
    // inst still owns the ranges, which smpl has no say in
    CHECK(z.mapping.keyboardRange.keyStart == 30);
    CHECK(z.mapping.keyboardRange.keyEnd == 70);
    CHECK(z.mapping.velocityRange.velStart == 10);
    CHECK(z.mapping.velocityRange.velEnd == 100);
}

TEST_CASE("WAV inst alone carries root key and fine tune", "[sample][finetune]")
{
    for (int8_t tune : {(int8_t)-12, (int8_t)0, (int8_t)33})
    {
        DYNAMIC_SECTION("fine tune " << (int)tune)
        {
            auto p = tuneDir("inst") / "s.wav";
            writeWav(p, 1000, std::nullopt, InstSpec{.rootKey = 64, .fineTune = tune});

            Fixture f;
            f.load(p);
            auto &z = f.onlyZone();

            const auto &m = sampleOf(z).meta;
            CHECK((int)m.key_root == 64);
            CHECK(m.detune == Approx((float)tune));

            CHECK(z.mapping.rootKey == 64);
            CHECK(z.mapping.pitchOffset == Approx(tune / 100.f).margin(1e-6));
        }
    }
}

TEST_CASE("Browser loads take the fine tune only with the sample's root", "[sample][finetune]")
{
    auto p = tuneDir("browser") / "s.wav";
    writeWav(p, 1000, SmplSpec{52, {}, fractionFor(90.0)});

    SECTION("sample range info wins")
    {
        Fixture f;
        f.th.sendToSerialization(
            cmsg::AddSamples({{{p.u8string(), 60, 0, 127, 0, 127, true}}, -1, -1}));
        f.th.sync();
        auto &z = f.onlyZone();
        CHECK(z.mapping.rootKey == 53);
        CHECK(z.mapping.pitchOffset == Approx(0.1f).margin(1e-6));
    }

    SECTION("dropped on a key keeps that key untuned")
    {
        Fixture f;
        f.th.sendToSerialization(
            cmsg::AddSamples({{{p.u8string(), 67, 60, 72, 0, 127, false}}, -1, -1}));
        f.th.sync();
        auto &z = f.onlyZone();
        CHECK(z.mapping.rootKey == 67);
        CHECK(z.mapping.pitchOffset == 0.f);
    }
}

TEST_CASE("SFZ tune stacks on the sample fine tune", "[sample][finetune][sfz]")
{
    const WavSpec flat{"flat.wav", 1000, SmplSpec{52, {}, fractionFor(90.0)}};

    SECTION("keycenter from the sample")
    {
        auto p = buildSfz("ft_keycenter_sample",
                          "<region> sample=flat.wav pitch_keycenter=sample tune=4\n", {flat});
        Fixture f;
        f.load(p);
        CHECK(f.onlyZone().mapping.rootKey == 53);
        CHECK(f.onlyZone().mapping.pitchOffset == Approx(0.14f).margin(1e-6));
    }

    SECTION("explicit keycenter ignores the sample")
    {
        auto p = buildSfz("ft_keycenter_60", "<region> sample=flat.wav pitch_keycenter=60 tune=4\n",
                          {flat});
        Fixture f;
        f.load(p);
        CHECK(f.onlyZone().mapping.rootKey == 60);
        CHECK(f.onlyZone().mapping.pitchOffset == Approx(0.04f).margin(1e-6));
    }
}

#if SCXT_USE_FLAC
namespace
{
FLAC__StreamEncoderWriteStatus flacTuneWrite(const FLAC__StreamEncoder *, const FLAC__byte buffer[],
                                             size_t bytes, uint32_t, uint32_t, void *ud)
{
    auto *out = static_cast<std::vector<uint8_t> *>(ud);
    out->insert(out->end(), buffer, buffer + bytes);
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

// a mono flac with a riff application block holding one smpl chunk
std::vector<uint8_t> flacWithSmpl(uint32_t unity, uint32_t fraction)
{
    scxt::sample::loaders::SamplerChunk chunk{};
    chunk.dwMIDIUnityNote = (int32_t)unity;
    chunk.dwMIDIPitchFraction = (int32_t)fraction;

    std::vector<uint8_t> app;
    putTag(app, "smpl");
    putU32(app, sizeof(chunk));
    auto *cb = reinterpret_cast<const uint8_t *>(&chunk);
    app.insert(app.end(), cb, cb + sizeof(chunk));

    auto *block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_APPLICATION);
    REQUIRE(block);
    std::memcpy(block->data.application.id, "riff", 4);
    REQUIRE(
        FLAC__metadata_object_application_set_data(block, app.data(), (unsigned)app.size(), true));

    std::vector<FLAC__int32> samples(256, 0);
    std::vector<uint8_t> out;
    auto *enc = FLAC__stream_encoder_new();
    REQUIRE(enc);
    FLAC__stream_encoder_set_channels(enc, 1);
    FLAC__stream_encoder_set_bits_per_sample(enc, 16);
    FLAC__stream_encoder_set_sample_rate(enc, 48000);
    FLAC__stream_encoder_set_total_samples_estimate(enc, samples.size());
    FLAC__stream_encoder_set_metadata(enc, &block, 1);
    REQUIRE(FLAC__stream_encoder_init_stream(enc, flacTuneWrite, nullptr, nullptr, nullptr, &out) ==
            FLAC__STREAM_ENCODER_INIT_STATUS_OK);
    REQUIRE(FLAC__stream_encoder_process_interleaved(enc, samples.data(), samples.size()));
    REQUIRE(FLAC__stream_encoder_finish(enc));
    FLAC__stream_encoder_delete(enc);
    FLAC__metadata_object_delete(block);
    return out;
}
} // namespace

TEST_CASE("FLAC smpl fine tune folds like WAV", "[sample][finetune][flac]")
{
    for (const auto &c : smplCases)
    {
        DYNAMIC_SECTION(c.name)
        {
            auto f = flacWithSmpl(c.unity, fractionFor(c.fractionCents));
            scxt::sample::Sample s;
            REQUIRE(s.parseFlac(f.data(), f.size()));
            CHECK(s.meta.rootkey_present);
            CHECK((int)s.meta.key_root == c.root);
            CHECK(s.meta.detune == Approx(c.cents).margin(1e-4));
        }
    }
}
#endif
