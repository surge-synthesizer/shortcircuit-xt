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
#include "json/engine_traits.h"
#include "json/dsp_traits.h"
#include "json/modulation_traits.h"

using namespace scxt;

template <typename T> std::string testStream(const T &in)
{
    return tao::json::to_string(scxt::json::scxt_value(in));
}

template <typename T> void testUnstream(const std::string &s, T &in)
{
    tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>>
        consumer;
    tao::json::events::from_string(consumer, s);
    auto val = std::move(consumer.value);

    val.to(in);
}

TEST_CASE("Stream a engine::KeyboardRange")
{
    SECTION("Compiles")
    {
        engine::KeyboardRange k1, k2;
        auto s = testStream(k1);
        testUnstream(s, k2);
    }

    SECTION("Values")
    {
        engine::KeyboardRange k1, k2;
        k1.keyStart = 13;
        k1.keyEnd = 19;
        k1.fadeStart = 2;
        k1.fadeEnd = 7;

        REQUIRE(k1 != k2);
        auto s = testStream(k1);
        testUnstream(s, k2);
        REQUIRE(k1 == k2);
    }
}

TEST_CASE("Stream a dsp::filter::ProcessorStorage")
{
    SECTION("Compiles")
    {
        dsp::processor::ProcessorStorage k1, k2;
        auto s = testStream(k1);
        testUnstream(s, k2);
    }

    SECTION("Type Streams")
    {
        dsp::processor::ProcessorStorage k1, k2;
        k1.type = dsp::processor::proct_osc_EBWaveforms;
        REQUIRE(k1 != k2);
        auto s = testStream(k1);
        testUnstream(s, k2);
        REQUIRE(k1 == k2);
    }

    SECTION("Expanded Values Stream")
    {
        dsp::processor::ProcessorStorage k1, k2;
        k1.type = dsp::processor::proct_osc_EBWaveforms;
        k1.mix = 0.23;
        for (auto &fv : k1.floatParams)
            fv = 1.0 * (rand() % 10000) / 7842.2;
        for (auto &iv : k1.intParams)
            iv = rand() % 38;
        REQUIRE(k1 != k2);
        auto s = testStream(k1);
        testUnstream(s, k2);
        REQUIRE(k1 == k2);
    }
}

TEST_CASE("Stream modulation::modulators::StepLFOStorage")
{
    SECTION("Compiles")
    {
        modulation::modulators::StepLFOStorage k1, k2;
        auto s = testStream(k1);
        testUnstream(s, k2);
    }
}

TEST_CASE("Stream engine::Zone")
{
    SECTION("Compiles")
    {
        engine::Zone k1, k2;
        auto s = testStream(k1);
        testUnstream(s, k2);
    }

    SECTION("Sends a Mod")
    {
#if BADBAD
        engine::Zone k1, k2;
        k1.routingTable[3].src = scxt::modulation::vms_LFO2;
        k1.routingTable[3].dst = {scxt::modulation::vmd_Processor_Mix, 0};
        k1.routingTable[3].depth = 0.24;
        REQUIRE(k1 != k2);
        auto s = testStream(k1);
        testUnstream(s, k2);
        REQUIRE(k1 == k2);
#endif
    }

    SECTION("Overwrites a Mod")
    {
#if BADBAD
        engine::Zone k1, k2;
        k1.routingTable[3].src = scxt::modulation::vms_LFO2;
        k1.routingTable[3].dst = {scxt::modulation::vmd_Processor_Mix, 0};
        k1.routingTable[3].depth = 0.24;

        k2.routingTable[4].dst = {scxt::modulation::vmd_LFO_Rate, 0};
        REQUIRE(k1 != k2);
        auto s = testStream(k1);
        testUnstream(s, k2);
        REQUIRE(k1 == k2);
#endif
    }

    // TODO: Expand this test
}

// TODO: Add test for Group streaming
// TODO: Add test for Part streaming
// TODO: Add test for Patch streaming
// TODO: Add test for Engine streaming and Sample Library
