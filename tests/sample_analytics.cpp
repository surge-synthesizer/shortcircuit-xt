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
#include "dsp/sample_analytics.h"
#include <limits>
#include <cmath>

TEST_CASE("Sample Analytics", "[sample]")
{
    float _scratch; // Create a scratch double for modf

    // SineBuffer is a 440Hz size at 0.6 amplitude
    // sine rms = amp / sqrt(2)
    std::array<float, 1024> sineBuffer{};
    constexpr float sine_amp = 0.6f;
    const float sine_rms = sine_amp / sqrt(2.0f);
    const auto sineSample = std::make_shared<scxt::sample::Sample>();
    sineSample->allocateF32(0, sineBuffer.size());
    for (int i = 0; i < sineBuffer.size(); i++)
    {
        const float t = float(i) / sineBuffer.size();
        sineBuffer[i] = 0.6f * std::sin(2.0f * float(M_PI) * 440.0f * t);
    }
    sineSample->load_data_f32(0, sineBuffer.data(), sineBuffer.size(), sizeof(float));
    sineSample->sampleLengthPerChannel = sineBuffer.size();
    sineSample->channels = 1;
    sineSample->sample_loaded = true;

    // SquareBuffer is a 220Hz square wave at 0.8 amplitude
    // square rms = amp
    std::array<float, 1024> squareBuffer{};
    constexpr float square_amp = 0.8f;
    constexpr float square_rms = square_amp;
    const auto squareSample = std::make_shared<scxt::sample::Sample>();
    squareSample->allocateF32(0, squareBuffer.size());
    for (int i = 0; i < squareBuffer.size(); i++)
    {
        const float t = float(i) / squareBuffer.size();
        squareBuffer[i] = 0.8f * (std::modf(220.0f * t, &_scratch) > 0.5 ? -1.0f : 1.0f);
    }
    squareSample->load_data_f32(0, squareBuffer.data(), squareBuffer.size(), sizeof(float));
    squareSample->sampleLengthPerChannel = squareBuffer.size();
    squareSample->channels = 1;
    squareSample->sample_loaded = true;

    // SawBuffer is a 110Hz saw wave at 0.3 amplitude in stereo using i16s to sample
    // saw rms = amp / sqrt(3)
    std::array<int16_t, 1024> sawBuffer{};
    constexpr float saw_amp = 0.3f;
    const float saw_rms = saw_amp / sqrt(3.0f);
    const auto sawSample = std::make_shared<scxt::sample::Sample>();
    sawSample->allocateI16(0, sawBuffer.size());
    sawSample->allocateI16(1, sawBuffer.size());
    for (int i = 0; i < sawBuffer.size(); i++)
    {
        const float t = float(i) / sawBuffer.size();
        sawBuffer[i] = static_cast<int16_t>((0.6f * std::modf(110.0f * t, &_scratch) - 0.3f) *
                                            std::numeric_limits<int16_t>::max());
    }
    sawSample->load_data_i16(0, sawBuffer.data(), sawBuffer.size(), sizeof(int16_t));
    sawSample->load_data_i16(1, sawBuffer.data(), sawBuffer.size(), sizeof(int16_t));
    sawSample->sampleLengthPerChannel = sawBuffer.size();
    sawSample->channels = 2;
    sawSample->sample_loaded = true;

    // The smallest tolerance before test failure (only tested factors of 10, so might be able
    // to go smaller).
    constexpr float tolerance = 0.0001f;

    SECTION("Peak Analysis")
    {
        REQUIRE_THAT(scxt::dsp::sample_analytics::computePeak(sineSample),
                     Catch::WithinRel(sine_amp, tolerance));
        REQUIRE_THAT(scxt::dsp::sample_analytics::computePeak(squareSample),
                     Catch::WithinRel(square_amp, tolerance));
        REQUIRE_THAT(scxt::dsp::sample_analytics::computePeak(sawSample),
                     Catch::WithinRel(saw_amp, tolerance));
    }

    SECTION("RMS Analysis")
    {
        REQUIRE_THAT(scxt::dsp::sample_analytics::computeRMS(sineSample),
                     Catch::WithinRel(sine_rms, tolerance));
        REQUIRE_THAT(scxt::dsp::sample_analytics::computeRMS(squareSample),
                     Catch::WithinRel(square_rms, tolerance));
        REQUIRE_THAT(scxt::dsp::sample_analytics::computeRMS(sawSample),
                     Catch::WithinRel(saw_rms, tolerance));
    }
}

TEST_CASE("Nearest Zero Crossing", "[sample]")
{
    using scxt::dsp::sample_analytics::nearestZeroCrossing;

    auto makeF32 = [](const std::vector<float> &d) {
        auto s = std::make_shared<scxt::sample::Sample>();
        s->allocateF32(0, d.size());
        auto buf = d;
        s->load_data_f32(0, buf.data(), buf.size(), sizeof(float));
        s->sampleLengthPerChannel = d.size();
        s->channels = 1;
        s->sample_loaded = true;
        return s;
    };

    // sign changes between 2/3 (3 is quieter) and 6/7 (6 is quieter)
    auto s = makeF32({0.5f, 0.4f, 0.3f, -0.1f, -0.4f, -0.5f, -0.2f, 0.6f, 0.7f, 0.8f});
    auto len = (int64_t)s->getSampleLength();

    SECTION("Picks the quieter side of the nearest sign change")
    {
        REQUIRE(nearestZeroCrossing(s, 0, 0, len) == 3);
        REQUIRE(nearestZeroCrossing(s, 3, 0, len) == 3);
        REQUIRE(nearestZeroCrossing(s, 8, 0, len) == 6);
        // 4 is one from 3 and two from 6
        REQUIRE(nearestZeroCrossing(s, 4, 0, len) == 3);
        REQUIRE(nearestZeroCrossing(s, 5, 0, len) == 6);
    }

    SECTION("Stays inside the range it is given")
    {
        REQUIRE(nearestZeroCrossing(s, 4, 5, len) == 6);
        REQUIRE(nearestZeroCrossing(s, 5, 0, 4) == 3);
        REQUIRE(nearestZeroCrossing(s, 8, 7, len) == -1);
        REQUIRE(nearestZeroCrossing(s, 4, 6, 2) == -1);
    }

    SECTION("An exact zero is a crossing")
    {
        auto z = makeF32({0.5f, 0.5f, 0.0f, 0.5f, 0.5f});
        REQUIRE(nearestZeroCrossing(z, 4, 0, 5) == 2);
    }

    SECTION("A signal that never crosses has no crossing")
    {
        auto dc = makeF32({0.2f, 0.3f, 0.4f, 0.3f, 0.2f});
        REQUIRE(nearestZeroCrossing(dc, 2, 0, 5) == -1);
    }

    SECTION("Stereo crosses where the channels sum to a crossing")
    {
        auto st = std::make_shared<scxt::sample::Sample>();
        std::array<int16_t, 6> l{100, 100, 100, 100, 100, 100};
        std::array<int16_t, 6> r{50, 20, -50, -150, -300, -300};
        st->allocateI16(0, l.size());
        st->allocateI16(1, r.size());
        st->load_data_i16(0, l.data(), l.size(), sizeof(int16_t));
        st->load_data_i16(1, r.data(), r.size(), sizeof(int16_t));
        st->sampleLengthPerChannel = l.size();
        st->channels = 2;
        st->sample_loaded = true;

        // sums are 150 120 50 -50 -200 -200: the change is between 2 and 3, a tie
        auto c = nearestZeroCrossing(st, 5, 0, 6);
        REQUIRE(c == 3);
        REQUIRE(nearestZeroCrossing(st, 0, 0, 6) == 2);
    }
}
