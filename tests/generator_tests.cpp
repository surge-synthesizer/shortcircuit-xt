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

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <vector>

#include "dsp/data_tables.h"
#include "dsp/generator.h"
#include "dsp/resampling.h"
#include "engine/engine.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "messaging/messaging.h"
#include "voice/voice.h"

#include "test_utils.h"

// named rather than anonymous so the unity build cannot collide with other files' helpers
namespace generator_test
{
namespace dsp = scxt::dsp;

// one mono float generator looping over a buffer with the zero pads a real sample has
struct LoopedGenerator
{
    static constexpr int pad{dsp::FIRoffset};
    std::vector<float> buffer;
    dsp::GeneratorState GD;
    dsp::GeneratorIO IO;
    float outL[2 * scxt::blockSize]{}, outR[2 * scxt::blockSize]{};
    bool forward{true};
    bool whileGated{false};

    LoopedGenerator(int fileLength, int loopStart, int loopLength, bool fwd = true)
        : buffer(pad + fileLength + pad, 0.f), forward(fwd)
    {
        dsp::sincTable.init();

        IO.sampleDataL = sample();
        IO.sampleDataR = sample();
        IO.outputL = outL;
        IO.outputR = outR;
        IO.waveSize = fileLength;

        GD.samplePos = loopStart;
        GD.playbackLowerBound = 0;
        GD.playbackUpperBound = fileLength;
        GD.loopLowerBound = loopStart;
        GD.loopUpperBound = loopStart + loopLength;
        GD.loopInvertedBounds = 1.f / loopLength;
        GD.direction = 1;
        GD.directionAtOutset = 1;
        GD.isFinished = false;
        GD.gated = true;
    }

    float *sample() { return buffer.data() + pad; }

    void fillWithSine(int cycleLength)
    {
        for (int i = 0; i < IO.waveSize; ++i)
            sample()[i] = std::sin(2.0 * M_PI * i / cycleLength);
    }

    // the output then traces the playhead
    void fillWithRamp()
    {
        for (int i = 0; i < IO.waveSize; ++i)
            sample()[i] = i * 1e-3f;
    }

    void setRatio(double r) { GD.ratio = (int64_t)std::llround(r * (1 << 24)); }

    std::vector<float> render(int samples)
    {
        auto gen = dsp::GetFPtrGeneratorSample(false, true, true, forward, whileGated);
        std::vector<float> res;
        while ((int)res.size() < samples)
        {
            gen(&GD, &IO);
            res.insert(res.end(), outL, outL + GD.blockSize);
        }
        res.resize(samples);
        return res;
    }
};

// upward crossings of the mean, which is the fundamental for the simple shapes used here
double cyclesPerSample(const std::vector<float> &v, int skip)
{
    double mean{0};
    for (int i = skip; i < (int)v.size(); ++i)
        mean += v[i];
    mean /= (v.size() - skip);

    int crossings{0};
    for (int i = skip + 1; i < (int)v.size(); ++i)
        if (v[i - 1] <= mean && v[i] > mean)
            crossings++;
    return (double)crossings / (v.size() - skip);
}

struct RootZeroZone
{
    std::unique_ptr<scxt::engine::Engine> eng;
    scxt::engine::Zone *zone{nullptr};

    RootZeroZone()
    {
        eng.reset(makeEngine());
        auto &part = *eng->getPatch()->getPart(0);
        part.addGroup();

        auto z = std::make_unique<scxt::engine::Zone>();
        z->mapping.keyboardRange = {0, 127};
        z->mapping.velocityRange = {0, 127};
        z->mapping.rootKey = 0;
        z->initialize();
        part.getGroup(0)->addZone(z);
        zone = part.getGroup(0)->getZone(0).get();

        auto p = samplePath("WavStereo48k.wav");
        REQUIRE(std::filesystem::exists(p));

        auto bypass = eng->getMessageController()->threadingChecker.bypassChecksInScope();
        auto sid = eng->getSampleManager()->loadSampleByPath(p);
        REQUIRE(sid.has_value());
        zone->variantData.variants[0].sampleID = *sid;
        zone->variantData.variants[0].active = true;
        REQUIRE(zone->attachToSample(*eng->getSampleManager(), 0,
                                     scxt::engine::Zone::SampleInformationRead::ENDPOINTS));
    }

    double ratioForKey(int key)
    {
        eng->processNoteOnEvent(0, 0, key, -1, 1.f, 0.f);
        eng->processAudio();

        scxt::voice::Voice *newest{nullptr};
        for (int i = 0; i < (int)scxt::maxVoices; ++i)
        {
            auto *v = zone->voiceWeakPointers[i];
            if (v && v->isVoiceAssigned &&
                (!newest || v->voiceCreationId > newest->voiceCreationId))
                newest = v;
        }
        REQUIRE(newest);
        REQUIRE(newest->numGeneratorsActive == 1);
        auto res = (double)newest->GD[0].ratio;

        eng->processNoteOffEvent(0, 0, key, -1, 0.f);
        for (int i = 0; i < 200; ++i)
            eng->processAudio();
        return res;
    }
};

TEST_CASE("A single cycle keeps rising past seven octaves up", "[generator]")
{
    static constexpr int cycle{2048};

    SECTION("Zero order hold runs at the output rate")
    {
        LoopedGenerator g(cycle, 0, cycle);
        g.fillWithSine(cycle);
        g.GD.interpolationType = dsp::InterpolationTypes::ZeroOrderHold;
        g.setRatio(200);
        REQUIRE(cyclesPerSample(g.render(1 << 16), 512) == Approx(200.0 / cycle).epsilon(0.005));
    }

    SECTION("Sinc with the doubled block of an oversampled voice")
    {
        LoopedGenerator g(cycle, 0, cycle);
        g.fillWithSine(cycle);
        g.GD.blockSize = 2 * scxt::blockSize;
        g.setRatio(150);
        REQUIRE(cyclesPerSample(g.render(1 << 16), 512) == Approx(150.0 / cycle).epsilon(0.005));
    }
}

TEST_CASE("A single cycle shorter than the interpolator loops at pitch", "[generator]")
{
    auto cycle = GENERATE(8, 16, 23);
    INFO("cycle " << cycle);

    LoopedGenerator g(cycle, 0, cycle);
    g.fillWithSine(cycle);
    g.setRatio(0.1 * cycle);
    auto out = g.render(1 << 14);

    REQUIRE(cyclesPerSample(out, 256) == Approx(0.1).epsilon(0.005));

    float peak{0};
    for (auto s : out)
        peak = std::max(peak, std::fabs(s));
    REQUIRE(peak < 2.f);
}

TEST_CASE("A ping-pong loop plays at its true pitch", "[generator]")
{
    static constexpr int loopLength{64};

    // neither ratio divides the loop, so a stepped period shows
    auto ratio = GENERATE(5.5, 28.8);
    auto gated = GENERATE(false, true);
    INFO("ratio " << ratio << " gated " << gated);

    LoopedGenerator g(4096, 1024, loopLength, false);
    g.fillWithRamp();
    g.whileGated = gated;
    g.setRatio(ratio);
    REQUIRE(cyclesPerSample(g.render(1 << 16), 512) ==
            Approx(ratio / (2 * loopLength)).epsilon(0.005));
}

TEST_CASE("A ping-pong loop turns around at its bounds", "[generator]")
{
    LoopedGenerator g(4096, 1024, 64, false);
    g.fillWithRamp();
    g.setRatio(28.8);
    auto out = g.render(1 << 12);

    // a sample or two of slack for the interpolator reading behind the playhead
    auto [lo, hi] = std::minmax_element(out.begin() + 64, out.end());
    REQUIRE(*lo > 1021e-3f);
    REQUIRE(*hi < 1090e-3f);
}

TEST_CASE("A ping-pong loop counts each return to where it set out", "[generator]")
{
    LoopedGenerator g(4096, 1024, 64, false);
    g.fillWithRamp();

    // back at the start every 128 samples
    g.render(5 * 128 + 8);
    REQUIRE(g.GD.loopCount == 5);
}

TEST_CASE("A ping-pong loop counts every return when one step crosses it several times",
          "[generator]")
{
    static constexpr int loopLength{4};
    auto outset = GENERATE(1, -1);
    INFO("outset " << outset);

    LoopedGenerator g(4096, 1024, loopLength, false);
    g.fillWithRamp();
    g.setRatio(10.3);
    g.GD.direction = outset;
    g.GD.directionAtOutset = outset;
    g.GD.samplePos = outset > 0 ? g.GD.loopLowerBound : g.GD.loopUpperBound;
    // already in the loop, so skip the not yet looped -1
    g.GD.loopCount = 0;

    static constexpr int steps{256};
    g.render(steps);

    // back at the outset bound after every whole round trip travelled
    auto travelled = (int64_t)steps * g.GD.ratio;
    REQUIRE(g.GD.loopCount == travelled / ((int64_t)2 * loopLength << 24));
}

TEST_CASE("A ping-pong playhead outside its loop heads back rather than jumping in", "[generator]")
{
    LoopedGenerator g(4096, 1024, 64, false);
    g.fillWithRamp();
    g.GD.samplePos = 2000;
    auto out = g.render(2048);

    REQUIRE(out[16] > 1.9f);
    REQUIRE(out[256] < out[16]);

    auto [lo, hi] = std::minmax_element(out.begin() + 1024, out.end());
    REQUIRE(*lo > 1021e-3f);
    REQUIRE(*hi < 1090e-3f);
}

TEST_CASE("A key eight octaves above the root keeps its full ratio", "[generator]")
{
    RootZeroZone f;
    auto twoOctaves = f.ratioForKey(24);
    REQUIRE(twoOctaves > 0);
    REQUIRE(f.ratioForKey(96) == Approx(64.0 * twoOctaves).epsilon(1e-5));
}
} // namespace generator_test
