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

// GH #2382: switches go through real messages, tooltips are built from what the mod pane receives

#include "catch2/catch2.hpp"

#include <cmath>
#include <iostream>
#include <sstream>

#include "console_harness.h"
#include "dsp/processor/processor.h"
#include "dsp/processor/processor_impl.h"
#include "engine/engine.h"
#include "engine/part.h"
#include "modulation/voice_matrix.h"
#include "selection/selection_manager.h"
#include "sst/voice-effects/generator/SinePlus.h"

namespace tooltip_switch
{
namespace cmsg = scxt::messaging::client;
namespace vm = scxt::voice::modulation;
using pmd = scxt::datamodel::pmd;
using PS = scxt::dsp::processor::ProcessorStorage;
using Adsr = scxt::modulation::modulators::AdsrStorage;
using SinePlus = sst::voice_effects::generator::SinePlus<scxt::dsp::processor::SCXTVFXConfig<1>>;

static constexpr int row{0};

struct Shown
{
    pmd md;
    float base{0};
    pmd::FeatureState fs;
    pmd::ModulationDisplay disp;
    std::string knob;

    std::string line() const
    {
        std::ostringstream oss;
        oss << disp.value << " | " << disp.valDown << " < " << disp.baseValue << " > " << disp.valUp
            << " (knob " << knob << ")";
        return oss.str();
    }
};

struct Fixture
{
    scxt::clients::console_ui::ConsoleHarness th;
    vm::MatrixEndpoints ep{nullptr};

    Fixture()
    {
        th.start();
        th.sync();
        send(cmsg::AddBlankZone({0, 0, 48, 60, 0, 127}));
        send(cmsg::ApplySelectActions({{0, 0, 0, true, true, true}}));
    }

    template <typename T> void send(const T &msg)
    {
        th.sendToSerialization(msg);
        th.sync();
    }

    // a switch has to restate the matrix itself, so forget the last one first
    template <typename T> void sendExpectingMatrix(const T &msg)
    {
        th.editor->lastZoneMatrix.reset();
        send(msg);
    }

    // the knob mouse up
    void endEdit() { sendExpectingMatrix(cmsg::EndEdit(true)); }

    scxt::engine::Zone &zone()
    {
        return *th.engine->getPatch()->getPart(0)->getGroup(0)->getZone(0);
    }

    void route(const vm::MatrixConfig::TargetIdentifier &target, float depth)
    {
        auto r = zone().routingTable.routes[row];
        r.active = true;
        r.source = vm::sourcesForScanning().macroSources.macros[0];
        r.target = target;
        r.depth = depth;
        sendExpectingMatrix(cmsg::UpdateZoneRoutingRow({row, r, true}));
    }

    void setProcFloat(int idx, float v)
    {
        send(cmsg::UpdateZoneOrGroupProcessorFloatValue(
            {true, 0, (ptrdiff_t)(offsetof(PS, floatParams) + idx * sizeof(float)), v}));
        endEdit();
    }
    void setProcInt(int idx, int32_t v)
    {
        sendExpectingMatrix(cmsg::UpdateZoneOrGroupProcessorInt32TValue(
            {true, 0, (ptrdiff_t)(offsetof(PS, intParams) + idx * sizeof(int32_t)), v}));
    }
    void setKeytrack(bool v)
    {
        sendExpectingMatrix(
            cmsg::UpdateZoneOrGroupProcessorBoolValue({true, 0, offsetof(PS, isKeytracked), v}));
    }
    void setAegFloat(ptrdiff_t off, float v)
    {
        send(cmsg::UpdateZoneOrGroupEGFloatValue({true, (size_t)0, off, v}));
        endEdit();
    }
    void setAegTemposync(bool v)
    {
        sendExpectingMatrix(cmsg::UpdateZoneOrGroupEGBoolValue(
            {true, (size_t)0, (ptrdiff_t)offsetof(Adsr, isTemposync), v}));
    }

    // what ModPane::updateTooltip builds from the routing payload
    Shown shown(bool bipolar = true)
    {
        REQUIRE(th.editor->lastZoneMatrix.has_value());
        const auto &r = th.editor->lastZoneMatrix->routes[row];
        REQUIRE(r.extraPayload.has_value());
        Shown s;
        s.md = r.extraPayload->targetMetadata;
        s.base = r.extraPayload->targetBaseValue;
        s.fs = r.extraPayload->targetFeatureState;
        auto d = s.md.modulationNaturalToString(s.base, r.depth * (s.md.maxVal - s.md.minVal),
                                                bipolar, s.fs);
        REQUIRE(d.has_value());
        s.disp = *d;
        s.knob = s.md.valueToString(s.base, s.fs).value_or("-");
        return s;
    }
};

// the pane's metadata is the one the knob uses, and the baseline is the knob's text
void requireMatchesKnob(const Shown &s, const pmd &knobMd, float knobValue,
                        const pmd::FeatureState &knobFs)
{
    INFO("tooltip " << s.line());
    REQUIRE(s.md.minVal == knobMd.minVal);
    REQUIRE(s.md.maxVal == knobMd.maxVal);
    REQUIRE(s.md.displayScale == knobMd.displayScale);
    REQUIRE(s.md.supportsQuantization() == knobMd.supportsQuantization());
    REQUIRE(s.fs.isTemposynced == knobFs.isTemposynced);
    REQUIRE(s.base == Approx(knobValue));
    REQUIRE(s.disp.baseValue == knobMd.valueToString(knobValue, knobFs).value_or("?"));
}

// the endpoints read in the knob's own units, clamped to the range
void requireEndpointsInRange(const Shown &s, float depth)
{
    INFO("tooltip " << s.line());
    auto mod = depth * (s.md.maxVal - s.md.minVal);
    auto strip = [](const std::string &x) {
        return (!x.empty() && (x[0] == '>' || x[0] == '<')) ? x.substr(1) : x;
    };
    auto up = std::clamp(s.base + mod, s.md.minVal, s.md.maxVal);
    auto dn = std::clamp(s.base - mod, s.md.minVal, s.md.maxVal);
    REQUIRE(strip(s.disp.valUp) == s.md.valueToString(up, s.fs).value_or("?"));
    REQUIRE(strip(s.disp.valDown) == s.md.valueToString(dn, s.fs).value_or("?"));
}

// the depth type-in reads back the depth the tooltip shows
void requireTypeInRoundTrips(const Shown &s)
{
    INFO("tooltip " << s.line());
    std::string em;
    auto back = s.md.modulationNaturalFromString(s.disp.value, s.base, em, s.fs);
    INFO("type-in error '" << em << "'");
    REQUIRE(back.has_value());
    auto again = s.md.modulationNaturalToString(s.base, *back, true, s.fs);
    REQUIRE(again.has_value());
    REQUIRE(again->value == s.disp.value);
}

const pmd &procMd(Fixture &f, int idx)
{
    return f.zone().processorDescription[0].floatControlDescriptions[idx];
}
float procVal(Fixture &f, int idx) { return f.zone().processorStorage[0].floatParams[idx]; }

// the units a time reads in; a temposynced time reads as a note value
std::string unitOf(const std::string &t)
{
    auto ends = [&t](const std::string &x) {
        return t.size() >= x.size() && t.compare(t.size() - x.size(), x.size(), x) == 0;
    };
    if (ends(" ms"))
        return "ms";
    if (ends(" s"))
        return "s";
    if (ends(" %"))
        return "%";
    for (auto n : {"note", "triplet", "dotted"})
        if (t.find(n) != std::string::npos)
            return "note";
    return "?";
}

void dump(const std::string &label, const Shown &s)
{
    std::cout << label << " : " << s.line() << std::endl;
}
} // namespace tooltip_switch

using namespace tooltip_switch;

TEST_CASE("Sine Plus overtone tooltip follows harmonic quantize", "[modulation][tooltips]")
{
    Fixture f;
    f.send(cmsg::SetSelectedProcessorType(
        {true, (int32_t)0, (int32_t)scxt::dsp::processor::proct_osc_sineplus}));
    REQUIRE(f.zone().processorStorage[0].intParams[SinePlus::ipQuantA] == 1);

    f.setProcFloat(SinePlus::fpOffsetA, 3.f);
    // 2.64 harmonics, so an endpoint that truncates instead of rounding shows
    static constexpr float depth{0.12f};
    f.route(f.ep.processorTarget[0].fpT[SinePlus::fpOffsetA], depth);

    auto quantized = [&f](const std::string &down, const std::string &base, const std::string &up) {
        auto s = f.shown();
        INFO("tooltip " << s.line());
        REQUIRE(s.md.supportsQuantization());
        REQUIRE(s.md.maxVal == Approx(24.f));
        requireMatchesKnob(s, procMd(f, SinePlus::fpOffsetA), procVal(f, SinePlus::fpOffsetA), {});
        requireEndpointsInRange(s, depth);
        requireTypeInRoundTrips(s);
        REQUIRE(s.disp.valDown == down);
        REQUIRE(s.disp.baseValue == base);
        REQUIRE(s.disp.valUp == up);
    };
    auto unquantized = [&f](float harmonic) {
        auto s = f.shown();
        INFO("tooltip " << s.line());
        REQUIRE(s.md.minVal == Approx(12.f));
        REQUIRE(s.base == Approx(12.f * std::log2(harmonic)));
        requireMatchesKnob(s, procMd(f, SinePlus::fpOffsetA), procVal(f, SinePlus::fpOffsetA), {});
        requireEndpointsInRange(s, depth);
        requireTypeInRoundTrips(s);
        for (const auto &t : {s.disp.value, s.disp.valDown, s.disp.baseValue, s.disp.valUp})
        {
            REQUIRE(t.find("semitones") != std::string::npos);
            REQUIRE(t.find("harmonic") == std::string::npos);
        }
    };

    quantized("<2nd harmonic", "3rd harmonic", "6th harmonic");
    f.setProcInt(SinePlus::ipQuantA, 0);
    unquantized(3.f);
    f.setProcInt(SinePlus::ipQuantA, 1);
    quantized("<2nd harmonic", "3rd harmonic", "6th harmonic");

    f.setProcFloat(SinePlus::fpOffsetA, 7.f);
    quantized("4th harmonic", "7th harmonic", "10th harmonic");
    f.setProcInt(SinePlus::ipQuantA, 0);
    unquantized(7.f);

    // B has its own switch and A's leaves it alone
    f.route(f.ep.processorTarget[0].fpT[SinePlus::fpOffsetB], depth);
    auto sb = f.shown();
    REQUIRE(sb.md.supportsQuantization());
    REQUIRE(sb.disp.baseValue == "12th harmonic");
}

TEST_CASE("Sine Plus root tooltip follows keytrack", "[modulation][tooltips]")
{
    Fixture f;
    f.send(cmsg::SetSelectedProcessorType(
        {true, (int32_t)0, (int32_t)scxt::dsp::processor::proct_osc_sineplus}));
    static constexpr float depth{0.1f};
    f.route(f.ep.processorTarget[0].fpT[SinePlus::fpBaseFrequency], depth);

    // restating keytrack is not a switch, so start from the other side
    auto was = f.zone().processorStorage[0].isKeytracked;
    for (auto kt : {!was, was, !was})
    {
        INFO("keytrack " << kt);
        f.setKeytrack(kt);
        auto s = f.shown();
        requireMatchesKnob(s, procMd(f, SinePlus::fpBaseFrequency),
                           procVal(f, SinePlus::fpBaseFrequency), {});
        requireEndpointsInRange(s, depth);
        requireTypeInRoundTrips(s);
    }
}

TEST_CASE("EG time tooltip follows temposync and the ms switch", "[modulation][tooltips]")
{
    struct Stage
    {
        const char *name;
        vm::MatrixConfig::TargetIdentifier target;
        ptrdiff_t off;
        float Adsr::*mem;
    };
    using EGT = vm::MatrixEndpoints::EGTarget;
    for (auto st : {Stage{"Attack", EGT::attackTId(0), offsetof(Adsr, a), &Adsr::a},
                    Stage{"Decay", EGT::decayTId(0), offsetof(Adsr, d), &Adsr::d},
                    Stage{"Release", EGT::releaseTId(0), offsetof(Adsr, r), &Adsr::r}})
    {
        INFO("AEG " << st.name);
        Fixture f;
        static constexpr float depth{0.1f};
        f.route(st.target, depth);

        auto knobMd =
            scxt::datamodel::describeValue(f.zone().egStorage[0], f.zone().egStorage[0].*(st.mem));

        struct Units
        {
            float value;
            const char *down, *base, *up, *depth;
        };
        // the depth from 0.6 lands past a second, so its top end switches to s
        for (auto u : {Units{0.2f, "ms", "ms", "ms", "ms"}, Units{0.6f, "ms", "ms", "s", "ms"},
                       Units{0.9f, "s", "s", "s", "s"}})
        {
            f.setAegFloat(st.off, u.value);
            for (auto ts : {true, false, true, false})
            {
                INFO("value " << u.value << " temposync " << ts);
                f.setAegTemposync(ts);
                REQUIRE(f.zone().egStorage[0].isTemposync == ts);

                auto s = f.shown();
                INFO("tooltip " << s.line());
                requireMatchesKnob(s, knobMd, u.value, pmd::FeatureState{}.withTemposync(ts));
                requireEndpointsInRange(s, depth);
                requireTypeInRoundTrips(s);

                if (ts)
                {
                    REQUIRE(unitOf(s.disp.value) == "%");
                    for (const auto &t : {s.disp.valDown, s.disp.baseValue, s.disp.valUp})
                        REQUIRE(unitOf(t) == "note");
                }
                else
                {
                    REQUIRE(unitOf(s.disp.valDown) == u.down);
                    REQUIRE(unitOf(s.disp.baseValue) == u.base);
                    REQUIRE(unitOf(s.disp.valUp) == u.up);
                    REQUIRE(unitOf(s.disp.value) == u.depth);
                }
            }
        }
    }
}

TEST_CASE("Dump switched modulation tooltips", "[.][tooltips]")
{
    {
        Fixture f;
        f.send(cmsg::SetSelectedProcessorType(
            {true, (int32_t)0, (int32_t)scxt::dsp::processor::proct_osc_sineplus}));
        f.setProcFloat(SinePlus::fpOffsetA, 3.f);
        f.route(f.ep.processorTarget[0].fpT[SinePlus::fpOffsetA], 0.12f);
        dump("Sine+ Tune A quantized", f.shown());
        f.setProcInt(SinePlus::ipQuantA, 0);
        dump("Sine+ Tune A unquantized", f.shown());
        f.setProcInt(SinePlus::ipQuantA, 1);
        dump("Sine+ Tune A requantized", f.shown());
    }
    {
        Fixture f;
        f.route(vm::MatrixEndpoints::EGTarget::attackTId(0), 0.1f);
        for (auto v : {0.2f, 0.6f, 0.9f})
        {
            f.setAegFloat(offsetof(Adsr, a), v);
            for (auto ts : {false, true, false})
            {
                f.setAegTemposync(ts);
                dump("AEG Attack " + std::to_string(v) + (ts ? " synced" : ""), f.shown());
            }
        }
    }
}
