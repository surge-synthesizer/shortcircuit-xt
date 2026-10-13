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

#include <algorithm>
#include <cmath>
#include <vector>

#include "catch2/catch2.hpp"
#include "dsp/processor/processor.h"
#include "engine/memory_pool.h"
#include "engine/engine.h"
#include "modulation/modulators/curvelfo.h"

namespace
{
// rising (dir > 0) or falling edges of a 0..1 gain, sub-block via the linear lerp; the first
// blocks are skipped as the gain lerp starts from unity
std::vector<double> edgeTimes(const std::vector<float> &g, int dir)
{
    std::vector<double> res;
    for (size_t i = 2 * scxt::blockSize; i < g.size(); ++i)
    {
        auto a = g[i - 1] - 0.5f, b = g[i] - 0.5f;
        if ((dir > 0 && a < 0 && b >= 0) || (dir < 0 && a >= 0 && b < 0))
            res.push_back((i - 1) + a / (a - b));
    }
    return res;
}

// spread of edge times about the ideal grid; block quantization alone keeps it under a block
double gridSpread(const std::vector<double> &edges, double idealPeriod)
{
    double lo{0}, hi{0};
    for (size_t k = 0; k < edges.size(); ++k)
    {
        auto err = edges[k] - edges[0] - k * idealPeriod;
        lo = std::min(lo, err);
        hi = std::max(hi, err);
    }
    return hi - lo;
}
} // namespace

TEST_CASE("Tremolo temposync square holds tempo", "[vfx-temposync]")
{
    namespace pdsp = scxt::dsp::processor;

    // the issue 1003 case: 1/4 note square at 147.3 bpm for 20 seconds
    auto sr = GENERATE(44100.0, 48000.0, 96000.0);
    auto bpm = GENERATE(147.3, 120.0, 93.7);
    static constexpr double seconds{20.0};

    DYNAMIC_SECTION("sr=" << sr << " bpm=" << bpm)
    {
        scxt::engine::Engine e;
        scxt::engine::MemoryPool mp;
        pdsp::ProcessorStorage ps;
        ps.type = pdsp::proct_Tremolo;
        ps.isTemposynced = true;

        alignas(16) uint8_t memory[pdsp::processorMemoryBufferSize];
        float pfp[pdsp::maxProcessorFloatParams]{};
        int ifp[pdsp::maxProcessorIntParams]{};

        auto p = pdsp::spawnProcessorInPlace(pdsp::proct_Tremolo, &e, &mp, memory,
                                             pdsp::processorMemoryBufferSize, ps, pfp, ifp, false,
                                             false);
        REQUIRE(p);

        double tempo{bpm};
        p->setSampleRate(sr);
        p->setTempoPointer(&tempo);
        p->init_params();

        // log2(2 Hz) is a 1/4 note at 120
        pfp[1] = 1.f; // fpRate
        pfp[2] = 1.f; // fpDepth
        ifp[0] = 0;   // ipHarmonic
        ifp[1] = 4;   // ipShape square
        ifp[2] = 0;   // ipStereo
        p->init();

        alignas(16) float inL[scxt::blockSize], inR[scxt::blockSize];
        alignas(16) float outL[scxt::blockSize], outR[scxt::blockSize];
        for (int i = 0; i < scxt::blockSize; ++i)
        {
            inL[i] = 1.f;
            inR[i] = 1.f;
        }

        auto blocks = (size_t)(seconds * sr / scxt::blockSize);
        std::vector<float> gain;
        gain.reserve(blocks * scxt::blockSize);
        for (size_t b = 0; b < blocks; ++b)
        {
            p->process_stereo(inL, inR, outL, outR, 0.f);
            for (int i = 0; i < scxt::blockSize; ++i)
                gain.push_back(outL[i]);
        }
        pdsp::unspawnProcessor(p);

        auto idealPeriod = 60.0 / bpm * sr;
        for (auto dir : {1, -1})
        {
            auto edges = edgeTimes(gain, dir);
            auto spread = gridSpread(edges, idealPeriod);

            INFO("sr=" << sr << " bpm=" << bpm << " dir=" << dir << " edges=" << edges.size()
                       << " ideal period=" << idealPeriod << " spread=" << spread << " smp ("
                       << spread / sr * 1000.0 << "ms)");
            REQUIRE(edges.size() >= (size_t)(seconds * bpm / 60.0) - 1);
            REQUIRE(spread <= scxt::blockSize);
        }
    }
}

TEST_CASE("Curve LFO temposync holds tempo", "[vfx-temposync]")
{
    namespace mod = scxt::modulation;

    auto sr = GENERATE(44100.0, 48000.0, 96000.0);
    auto bpm = GENERATE(147.3, 120.0, 93.7);
    static constexpr double seconds{20.0};

    DYNAMIC_SECTION("sr=" << sr << " bpm=" << bpm)
    {
        scxt::dsp::twoToTheXTable.init();
        sst::basic_blocks::dsp::RNG rng;
        mod::ModulatorStorage ms;
        ms.temposync = true;
        sst::basic_blocks::modulators::Transport td;
        td.tempo = bpm;

        mod::modulators::CurveLFO lfo(rng);
        lfo.setSampleRate(sr);
        lfo.assign(&ms, &td);
        lfo.attack(0.f, 0.f, mod::ModulatorStorage::LFO_SINE);

        auto blocks = (size_t)(seconds * sr / scxt::blockSize);
        for (size_t b = 0; b < blocks; ++b)
            lfo.process(1.f, 0.f, 0.f, 0.f, 0.f, 0.f, false, false, true);

        auto idealCycles = blocks * scxt::blockSize / sr * bpm / 60.0;
        auto driftSamples = (lfo.cyclesElapsed - idealCycles) * 60.0 / bpm * sr;
        INFO("sr=" << sr << " bpm=" << bpm << " cycles=" << lfo.cyclesElapsed
                   << " ideal=" << idealCycles << " drift=" << driftSamples << " smp ("
                   << driftSamples / sr * 1000.0 << "ms)");
        REQUIRE(std::fabs(driftSamples) < 1.0);
    }
}

TEST_CASE("Microgate temposync repeats hold tempo", "[vfx-temposync]")
{
    namespace pdsp = scxt::dsp::processor;

    auto sr = GENERATE(44100.0, 48000.0, 96000.0);
    auto bpm = GENERATE(147.3, 120.0, 93.7);
    static constexpr double seconds{20.0};
    static constexpr double toneHz{220.0};

    DYNAMIC_SECTION("sr=" << sr << " bpm=" << bpm)
    {
        scxt::engine::Engine e;
        scxt::engine::MemoryPool mp;
        pdsp::ProcessorStorage ps;
        ps.type = pdsp::proct_fx_microgate;
        ps.isTemposynced = true;

        alignas(16) uint8_t memory[pdsp::processorMemoryBufferSize];
        float pfp[pdsp::maxProcessorFloatParams]{};
        int ifp[pdsp::maxProcessorIntParams]{};

        auto p = pdsp::spawnProcessorInPlace(pdsp::proct_fx_microgate, &e, &mp, memory,
                                             pdsp::processorMemoryBufferSize, ps, pfp, ifp, false,
                                             false);
        REQUIRE(p);

        double tempo{bpm};
        p->setSampleRate(sr);
        p->setTempoPointer(&tempo);
        p->init_params();

        // log2(0.125s) is a 1/16 note at 120
        pfp[0] = 65.f;  // fpRepeats, infinite
        pfp[1] = -3.f;  // fpLoopLength
        pfp[2] = -40.f; // fpThreshold, arms on the first block
        p->init();

        // the rising amplitude makes each restart from the buffer start a sharp level drop
        auto toneAt = [sr](size_t i) {
            auto t = i / sr;
            return (float)(std::min(0.1 + 4.0 * t, 1.0) * std::sin(2.0 * M_PI * toneHz * t));
        };

        alignas(16) float inL[scxt::blockSize], inR[scxt::blockSize];
        alignas(16) float outL[scxt::blockSize], outR[scxt::blockSize];
        auto blocks = (size_t)(seconds * sr / scxt::blockSize);
        std::vector<float> out;
        out.reserve(blocks * scxt::blockSize);
        for (size_t b = 0; b < blocks; ++b)
        {
            for (int i = 0; i < scxt::blockSize; ++i)
            {
                inL[i] = toneAt(b * scxt::blockSize + i);
                inR[i] = inL[i];
            }
            p->process_stereo(inL, inR, outL, outR, 0.f);
            for (int i = 0; i < scxt::blockSize; ++i)
                out.push_back(outL[i]);
        }
        pdsp::unspawnProcessor(p);

        // peak per tone period; the buffer starts near 0.1 and is past 0.2 within 25ms
        auto window = (size_t)std::ceil(sr / toneHz);
        std::vector<double> restarts;
        float lastPeak{0};
        for (size_t w = 0; w + window <= out.size(); w += window)
        {
            float peak{0};
            for (size_t i = w; i < w + window; ++i)
                peak = std::max(peak, std::fabs(out[i]));
            if (peak < 0.2f && lastPeak >= 0.2f)
                restarts.push_back(w);
            lastPeak = peak;
        }

        auto idealLength = 0.125 * 120.0 / bpm * sr;
        auto n = restarts.size() - 1;
        auto err = restarts.back() - restarts.front() - n * idealLength;
        INFO("sr=" << sr << " bpm=" << bpm << " restarts=" << restarts.size()
                   << " ideal length=" << idealLength << " error after " << n << " repeats=" << err
                   << " smp, tone period=" << sr / toneHz);
        REQUIRE(restarts.size() >= (size_t)(seconds * sr / idealLength) - 2);
        // a zero crossing search plus window detection, never more than a couple of periods
        REQUIRE(std::fabs(err) < 2.0 * window);
    }
}
