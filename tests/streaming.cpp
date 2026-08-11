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
#include <tao/json/to_string.hpp>
#include <tao/json/from_string.hpp>
#include "json/engine_traits.h"
#include "json/dsp_traits.h"
#include "json/modulation_traits.h"

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

TEST_CASE("Stream a scxt::engine::KeyboardRange")
{
    SECTION("Compiles")
    {
        scxt::engine::KeyboardRange k1, k2;
        auto s = testStream(k1);
        testUnstream(s, k2);
    }

    SECTION("Values")
    {
        scxt::engine::KeyboardRange k1, k2;
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

TEST_CASE("Stream a scxt::dsp::filter::ProcessorStorage")
{
    SECTION("Compiles")
    {
        scxt::dsp::processor::ProcessorStorage k1, k2;
        auto s = testStream(k1);
        testUnstream(s, k2);
    }

    SECTION("Type Streams")
    {
        scxt::dsp::processor::ProcessorStorage k1, k2;
        k1.type = scxt::dsp::processor::proct_osc_EBWaveforms;
        REQUIRE(k1 != k2);
        auto s = testStream(k1);
        testUnstream(s, k2);
        REQUIRE(k1 == k2);
    }

    SECTION("Expanded Values Stream")
    {
        scxt::dsp::processor::ProcessorStorage k1, k2;
        k1.type = scxt::dsp::processor::proct_osc_EBWaveforms;
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

TEST_CASE("Stream scxt::modulation::modulators::StepLFOStorage")
{
    SECTION("Compiles")
    {
        scxt::modulation::modulators::StepLFOStorage k1, k2;
        auto s = testStream(k1);
        testUnstream(s, k2);
    }
}

TEST_CASE("Stream scxt::engine::Zone")
{
    SECTION("Compiles")
    {
        scxt::engine::Zone k1, k2;
        auto s = testStream(k1);
        testUnstream(s, k2);
    }

    SECTION("Sends a Mod")
    {
#if BADBAD
        scxt::engine::Zone k1, k2;
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
        scxt::engine::Zone k1, k2;
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

TEST_CASE("A one shot variant unstreams as a sample gated AEG")
{
    namespace mm = scxt::modulation::modulators;
    using pm_t = scxt::engine::Zone::PlayMode;

    scxt::engine::Zone k1;
    k1.variantData.variants[0].active = true;
    k1.variantData.variants[0].playMode = pm_t::ON_RELEASE;
    REQUIRE(k1.egStorage[0].gateMode == mm::AdsrStorage::GateMode::GATED);

    auto s = testStream(k1);
    auto at = s.find("onrelease");
    REQUIRE(at != std::string::npos);

    // what a pre-August-2026 patch with a one shot variant looks like
    auto old = s;
    old.replace(at, std::string("onrelease").size(), "oneshot");

    SECTION("A play mode we still have round trips untouched")
    {
        scxt::engine::Zone k2;
        testUnstream(s, k2);
        REQUIRE(k2.variantData.variants[0].playMode == pm_t::ON_RELEASE);
        REQUIRE(k2.egStorage[0].gateMode == mm::AdsrStorage::GateMode::GATED);
    }

    SECTION("A one shot variant lands on the AEG instead")
    {
        scxt::engine::Engine::UnstreamGuard sg(0x2026'07'24);

        scxt::engine::Zone k2;
        testUnstream(old, k2);
        // the play mode is gone, so it falls back to normal
        REQUIRE(k2.variantData.variants[0].playMode == pm_t::NORMAL);
        REQUIRE(k2.egStorage[0].gateMode == mm::AdsrStorage::GateMode::SAMPLE_GATED);
    }

    SECTION("Only an old stream is searched for it")
    {
        // no guard, so this reads as an in-process stream from this build
        scxt::engine::Zone k2;
        testUnstream(old, k2);
        REQUIRE(k2.egStorage[0].gateMode == mm::AdsrStorage::GateMode::GATED);

        // and neither is one streamed at or past the version that dropped it
        scxt::engine::Engine::UnstreamGuard sg(scxt::currentStreamingVersion);
        scxt::engine::Zone k3;
        testUnstream(old, k3);
        REQUIRE(k3.egStorage[0].gateMode == mm::AdsrStorage::GateMode::GATED);
    }
}

// TODO: Add test for Group streaming
// TODO: Add test for Part streaming
// TODO: Add test for Patch streaming
// TODO: Add test for Engine streaming and Sample Library
