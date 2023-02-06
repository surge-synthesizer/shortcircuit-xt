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
    tao::json::events::transformer<tao::json::events::to_basic_value<scxt::json::scxt_traits>> consumer;
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
        k1.type = dsp::processor::proct_osc_pulse_sync;
        REQUIRE(k1 != k2);
        auto s = testStream(k1);
        testUnstream(s, k2);
        REQUIRE(k1 == k2);
    }

    SECTION("Expanded Values Stream")
    {
        dsp::processor::ProcessorStorage k1, k2;
        k1.type = dsp::processor::proct_osc_pulse_sync;
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

TEST_CASE("Test modulation::VoiceModMatrix::Routing")
{
    SECTION("Compiles")
    {
        modulation::VoiceModMatrix::Routing k1, k2;
        auto s = testStream(k1);
        testUnstream(s, k2);
    }

    SECTION("Information Streams")
    {
        modulation::VoiceModMatrix::Routing k1, k2;
        k1.src = scxt::modulation::vms_LFO1;
        k1.dst = scxt::modulation::vmd_Processor2_Mix;
        k1.depth = 0.03;
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

    SECTION("Information Streams")
    {
        modulation::modulators::StepLFOStorage k1, k2;
        for (auto &d : k1.data)
            d = (rand() % 1840) / 1932.f;
        k1.repeat = 3;
        k1.rate = 0.32;
        k1.smooth = 0.87;
        k1.shuffle = 0.42;
        k1.temposync = !k1.temposync;
        k1.triggermode = 4;
        k1.cyclemode = !k1.cyclemode;
        k1.onlyonce = !k1.onlyonce;
        REQUIRE(k1 != k2);

        auto s = testStream(k1);
        testUnstream(s, k2);

        REQUIRE(k1 == k2);
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
        engine::Zone k1, k2;
        k1.routingTable[3].src = scxt::modulation::vms_LFO2;
        k1.routingTable[3].dst = scxt::modulation::vmd_Processor1_Mix;
        k1.routingTable[3].depth = 0.24;
        REQUIRE(k1 != k2);
        auto s = testStream(k1);
        testUnstream(s, k2);
        REQUIRE(k1 == k2);
    }

    SECTION("Overwrites a Mod")
    {
        engine::Zone k1, k2;
        k1.routingTable[3].src = scxt::modulation::vms_LFO2;
        k1.routingTable[3].dst = scxt::modulation::vmd_Processor1_Mix;
        k1.routingTable[3].depth = 0.24;

        k2.routingTable[4].dst = scxt::modulation::vmd_LFO1_Rate;
        REQUIRE(k1 != k2);
        auto s = testStream(k1);
        testUnstream(s, k2);
        REQUIRE(k1 == k2);
    }

    // TODO: Expand this test
}

// TODO: Add test for Group streaming
// TODO: Add test for Part streaming
// TODO: Add test for Patch streaming
// TODO: Add test for Engine streaming and Sample Library