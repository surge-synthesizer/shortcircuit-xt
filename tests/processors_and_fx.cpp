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

#include "catch2/catch2.hpp"
#include "dsp/processor/processor.h"
#include "engine/memory_pool.h"
#include "engine/engine.h"

TEST_CASE("Processors have Formatting")
{
    namespace pdsp = scxt::dsp::processor;
    scxt::engine::Engine e;
    scxt::engine::MemoryPool mp;
    pdsp::ProcessorStorage procStorage;
    uint8_t memory[pdsp::processorMemoryBufferSize];
    float pfp[pdsp::maxProcessorFloatParams];
    int ifp[pdsp::maxProcessorIntParams];

    // start from 1 to skip proct_none
    for (int i = 1; i < pdsp::proct_num_types; ++i)
    {
        auto pt = (pdsp::ProcessorType)i;
        if (pdsp::isProcessorImplemented(pt))
        {
            DYNAMIC_SECTION("Formatting for " << pdsp::getProcessorName(pt))
            {
                REQUIRE(true);
                memset(memory, 0, sizeof(memory));
                memset(pfp, 0, sizeof(pfp));
                memset(ifp, 0, sizeof(ifp));
                procStorage.type = pt;
                auto p = pdsp::spawnProcessorInPlace(pt, &e, &mp, memory,
                                                     pdsp::processorMemoryBufferSize, procStorage,
                                                     pfp, ifp, false, true);
                REQUIRE(p);

                auto desc = p->getControlDescription();
                for (int f = 0; f < desc.numFloatParams; ++f)
                {
                    INFO(pdsp::getProcessorName(pt)
                         << " Float param " << f << " " << desc.floatControlDescriptions[f].name);
                    REQUIRE(desc.floatControlDescriptions[f].supportsStringConversion);
                }
                for (int f = 0; f < desc.numIntParams; ++f)
                {
                    INFO(pdsp::getProcessorName(pt)
                         << " Int param " << f << "/" << desc.numIntParams << " '"
                         << desc.intControlDescriptions[f].name << "' "
                         << desc.intControlDescriptions[f].type);
                    REQUIRE(desc.intControlDescriptions[f].supportsStringConversion);
                }
                pdsp::unspawnProcessor(p);
            }
        }
    }
}

TEST_CASE("Processor Short Names")
{
    namespace pdsp = scxt::dsp::processor;

    SECTION("Fallback algorithm")
    {
        // Drop vowels, take first 4, uppercase: "Treemonster" -> "TRMN"
        REQUIRE(pdsp::computeFallbackShortName("Treemonster") == "TRMN");
        // Spaces stripped, vowels dropped: "Auto Wah" -> "TWH" -> pads with first-4 alphanum
        // consonants = "tWh" (3 chars) -> not >=4 -> fall back to first-4 alphanum "AutoW" ->
        // "AUTO"
        REQUIRE(pdsp::computeFallbackShortName("Auto Wah") == "AUTO");
        // Mixed: "Bitcrusher" -> consonants "Btcrshr" -> "BTCR"
        REQUIRE(pdsp::computeFallbackShortName("Bitcrusher") == "BTCR");
        // Short input: "EQ" -> "EQ"
        REQUIRE(pdsp::computeFallbackShortName("EQ") == "EQ");
        // Empty/null
        REQUIRE(pdsp::computeFallbackShortName(nullptr) == "ERR");
    }

    SECTION("Every implemented processor has a non-empty short name")
    {
        for (int i = 1; i < pdsp::proct_num_types; ++i)
        {
            auto pt = (pdsp::ProcessorType)i;
            if (pdsp::isProcessorImplemented(pt))
            {
                DYNAMIC_SECTION("Short name for " << pdsp::getProcessorName(pt))
                {
                    auto sn = pdsp::getProcessorShortName(pt);
                    INFO("Processor: " << pdsp::getProcessorName(pt) << " short=" << sn);
                    REQUIRE(!sn.empty());
                    REQUIRE(sn.size() <= 4);
                }
            }
        }
    }
}

TEST_CASE("Dump Processor Silence Lengths")
{
    namespace pdsp = scxt::dsp::processor;
    scxt::engine::Engine e;
    scxt::engine::MemoryPool mp;
    pdsp::ProcessorStorage procStorage;
    uint8_t memory[pdsp::processorMemoryBufferSize];
    float pfp[pdsp::maxProcessorFloatParams];
    int ifp[pdsp::maxProcessorIntParams];

    // start from 1 to skip proct_none
    for (int i = 1; i < pdsp::proct_num_types; ++i)
    {
        auto pt = (pdsp::ProcessorType)i;
        if (pdsp::isProcessorImplemented(pt))
        {
            DYNAMIC_SECTION("Silence Length for " << pdsp::getProcessorName(pt))
            {
                REQUIRE(true);
                memset(memory, 0, sizeof(memory));
                memset(pfp, 0, sizeof(pfp));
                memset(ifp, 0, sizeof(ifp));
                procStorage.type = pt;
                auto p = pdsp::spawnProcessorInPlace(pt, &e, &mp, memory,
                                                     pdsp::processorMemoryBufferSize, procStorage,
                                                     pfp, ifp, false, true);
                p->init_params();
                p->init();

                REQUIRE(p);

                std::cout << "Processor " << p->getName() << " has silence length at default "
                          << p->silence_length() << std::endl;
                pdsp::unspawnProcessor(p);
            }
        }
    }
}
namespace
{
// rms of the last half of a run so filter startup does not count
double fast24RMS(int passband, int drive, int mode, float res, double freq)
{
    namespace pdsp = scxt::dsp::processor;
    static constexpr double sr{48000.0};

    scxt::engine::Engine e;
    scxt::engine::MemoryPool mp;
    pdsp::ProcessorStorage ps;
    ps.type = pdsp::proct_CytomicSVFCascade;

    alignas(16) uint8_t memory[pdsp::processorMemoryBufferSize];
    float pfp[pdsp::maxProcessorFloatParams]{};
    int ifp[pdsp::maxProcessorIntParams]{};

    auto p =
        pdsp::spawnProcessorInPlace(pdsp::proct_CytomicSVFCascade, &e, &mp, memory,
                                    pdsp::processorMemoryBufferSize, ps, pfp, ifp, false, false);
    REQUIRE(p);
    p->setSampleRate(sr);
    p->init_params();
    pfp[2] = res;
    ifp[1] = passband;
    ifp[3] = drive;
    ifp[4] = mode;
    p->init();

    alignas(16) float inL[scxt::blockSize], inR[scxt::blockSize];
    alignas(16) float outL[scxt::blockSize], outR[scxt::blockSize];
    static constexpr int blocks{4096};
    double phase{0}, sum{0};
    int n{0};
    bool finite{true};
    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < scxt::blockSize; ++i)
        {
            inL[i] = inR[i] = 0.5f * std::sin(phase);
            phase += 2.0 * M_PI * freq / sr;
        }
        p->process_stereo(inL, inR, outL, outR, 0.f);
        for (int i = 0; i < scxt::blockSize; ++i)
        {
            finite = finite && std::isfinite(outL[i]) && std::isfinite(outR[i]);
            if (b >= blocks / 2)
            {
                sum += outL[i] * outL[i];
                n++;
            }
        }
    }
    pdsp::unspawnProcessor(p);
    REQUIRE(finite);
    return std::sqrt(sum / n);
}
} // namespace

TEST_CASE("Fast SVF (24db) Modes")
{
    // passband order is LP, HP, BP, Notch, Peak, Allpass
    SECTION("Every passband, drive and mode runs finite")
    {
        for (int pb = 0; pb < 6; ++pb)
            for (int dr = 0; dr < 2; ++dr)
                for (int md = 0; md < 2; ++md)
                {
                    INFO("passband=" << pb << " drive=" << dr << " mode=" << md);
                    fast24RMS(pb, dr, md, 0.7f, 220.0);
                }
    }

    SECTION("Compensated holds lowpass passband level")
    {
        for (int dr = 0; dr < 2; ++dr)
        {
            // driven self-oscillates above 0.8
            auto res = dr ? 0.6f : 0.9f;
            auto standard = fast24RMS(0, dr, 0, res, 55.0);
            auto compensated = fast24RMS(0, dr, 1, res, 55.0);
            INFO("drive=" << dr << " standard=" << standard << " compensated=" << compensated);
            REQUIRE(compensated > standard * 1.5);
        }
    }

    SECTION("Compensated is ignored off lowpass")
    {
        auto standard = fast24RMS(1, 0, 0, 0.9f, 1760.0);
        auto compensated = fast24RMS(1, 0, 1, 0.9f, 1760.0);
        REQUIRE(compensated == Approx(standard).margin(1e-6));
    }
}
