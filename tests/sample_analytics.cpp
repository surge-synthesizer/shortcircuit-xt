/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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
#include "dsp/sample_analytics.h"
#include <limits>
#include <cmath>

using namespace scxt;

TEST_CASE("Sample Analytics", "[sample]")
{
    float _scratch; // Create a scratch double for modf

    // SineBuffer is a 440Hz size at 0.6 amplitude
    // sine rms = amp / sqrt(2)
    std::array<float, 1024> sineBuffer{};
    constexpr float sine_amp = 0.6f;
    const float sine_rms = sine_amp / sqrt(2.0f);
    const auto sineSample = std::make_shared<sample::Sample>();
    sineSample->allocateF32(0, sineBuffer.size());
    for (int i = 0; i < sineBuffer.size(); i++)
    {
        const float t = float(i) / sineBuffer.size();
        sineBuffer[i] = 0.6f * std::sin(2.0f * float(M_PI) * 440.0f * t);
    }
    sineSample->load_data_f32(0, sineBuffer.data(), sineBuffer.size(), sizeof(float));
    sineSample->sample_length = sineBuffer.size();
    sineSample->channels = 1;
    sineSample->sample_loaded = true;

    // SquareBuffer is a 220Hz square wave at 0.8 amplitude
    // square rms = amp
    std::array<float, 1024> squareBuffer{};
    constexpr float square_amp = 0.8f;
    constexpr float square_rms = square_amp;
    const auto squareSample = std::make_shared<sample::Sample>();
    squareSample->allocateF32(0, squareBuffer.size());
    for (int i = 0; i < squareBuffer.size(); i++)
    {
        const float t = float(i) / squareBuffer.size();
        squareBuffer[i] = 0.8f * (std::modf(220.0f * t, &_scratch) > 0.5 ? -1.0f : 1.0f);
    }
    squareSample->load_data_f32(0, squareBuffer.data(), squareBuffer.size(), sizeof(float));
    squareSample->sample_length = squareBuffer.size();
    squareSample->channels = 1;
    squareSample->sample_loaded = true;

    // SawBuffer is a 110Hz saw wave at 0.3 amplitude in stereo using i16s to sample
    // saw rms = amp / sqrt(3)
    std::array<int16_t, 1024> sawBuffer{};
    constexpr float saw_amp = 0.3f;
    const float saw_rms = saw_amp / sqrt(3.0f);
    const auto sawSample = std::make_shared<sample::Sample>();
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
    sawSample->sample_length = sawBuffer.size();
    sawSample->channels = 2;
    sawSample->sample_loaded = true;

    // The smallest tolerance before test failure (only tested factors of 10, so might be able
    // to go smaller).
    constexpr float tolerance = 0.0001f;

    SECTION("Peak Analysis")
    {
        REQUIRE_THAT(dsp::sample_analytics::computePeak(sineSample),
                     Catch::WithinRel(sine_amp, tolerance));
        REQUIRE_THAT(dsp::sample_analytics::computePeak(squareSample),
                     Catch::WithinRel(square_amp, tolerance));
        REQUIRE_THAT(dsp::sample_analytics::computePeak(sawSample),
                     Catch::WithinRel(saw_amp, tolerance));
    }

    SECTION("RMS Analysis")
    {
        REQUIRE_THAT(dsp::sample_analytics::computeRMS(sineSample),
                     Catch::WithinRel(sine_rms, tolerance));
        REQUIRE_THAT(dsp::sample_analytics::computeRMS(squareSample),
                     Catch::WithinRel(square_rms, tolerance));
        REQUIRE_THAT(dsp::sample_analytics::computeRMS(sawSample),
                     Catch::WithinRel(saw_rms, tolerance));
    }
}
