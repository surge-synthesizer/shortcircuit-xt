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

#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#include "catch2/catch2.hpp"
#include "processor_iteration_support.h"
#include "datamodel/metadata.h"
#include "modulation/modulator_storage.h"
#include "modulation/group_matrix.h"
#include "modulation/voice_matrix.h"
#include "engine/group.h"
#include "engine/part.h"
#include "engine/zone.h"
#include "test_utils.h"

namespace
{
namespace pdsp = scxt::dsp::processor;
using pmd = scxt::datamodel::pmd;

// what ModPane::updateTooltip shows for one routing: depth row then down < base > up
std::string tooltipLine(const pmd &md, float base, float depth, bool bipolar,
                        const pmd::FeatureState &fs)
{
    auto v = md.modulationNaturalToString(base, depth * (md.maxVal - md.minVal), bipolar, fs);
    if (!v.has_value())
        return "<no modulation display>";
    std::ostringstream oss;
    oss << v->value << " | ";
    if (bipolar)
        oss << v->valDown << " < " << v->baseValue << " > " << v->valUp;
    else
        oss << v->baseValue << " > " << v->valUp;
    return oss.str();
}

std::vector<float> baseValuesFor(const pmd &md, float stored)
{
    auto rg = md.maxVal - md.minVal;
    std::vector<float> res{stored, md.minVal + 0.25f * rg, md.minVal + 0.75f * rg};
    if (md.supportsQuantization())
    {
        for (auto &b : res)
            b = std::clamp(md.quantize(b), md.minVal, md.maxVal);
    }
    return res;
}

bool looksBroken(const std::string &s, const pmd &md)
{
    if (s.empty())
        return true;
    for (auto bad : {"Error", "ERROR", "err", "nan", "NaN"})
        if (s.find(bad) != std::string::npos)
            return true;
    // -inf is how a cubic level spells silence, and some params name their max inf
    auto p = s.find("inf");
    if (p == std::string::npos || (p > 0 && s[p - 1] == '-'))
        return false;
    auto unmarked = (s[0] == '>' || s[0] == '<') ? s.substr(1) : s;
    return unmarked != md.valueToString(md.maxVal).value_or("");
}

// the shown strings are sane and the depth type-in reads back what it shows
void checkTooltip(const pmd &md, float base, float depth, bool bipolar, const pmd::FeatureState &fs)
{
    auto mod = depth * (md.maxVal - md.minVal);
    auto v = md.modulationNaturalToString(base, mod, bipolar, fs);
    INFO("base " << base << " depth " << depth << " bipolar " << bipolar << " temposync "
                 << fs.isTemposynced);
    REQUIRE(v.has_value());
    INFO("tooltip '" << tooltipLine(md, base, depth, bipolar, fs) << "'");
    REQUIRE(!looksBroken(v->value, md));
    REQUIRE(!looksBroken(v->baseValue, md));
    REQUIRE(!looksBroken(v->valUp, md));
    if (bipolar)
        REQUIRE(!looksBroken(v->valDown, md));
    REQUIRE(v->baseValue == md.valueToString(base, fs).value_or("-"));

    if (base + mod <= md.maxVal)
    {
        std::string em;
        auto back = md.modulationNaturalFromString(v->value, base, em, fs);
        INFO("type-in error '" << em << "'");
        REQUIRE(back.has_value());
        auto again = md.modulationNaturalToString(base, *back, bipolar, fs);
        REQUIRE(again.has_value());
        REQUIRE(again->value == v->value);
    }
}

struct EGTime
{
    std::string name;
    pmd md;
};

std::vector<EGTime> egTimes()
{
    scxt::modulation::modulators::AdsrStorage eg;
    namespace dm = scxt::datamodel;
    return {{"Delay", dm::describeValue(eg, eg.dly)},
            {"Attack", dm::describeValue(eg, eg.a)},
            {"Hold", dm::describeValue(eg, eg.h)},
            {"Decay", dm::describeValue(eg, eg.d)},
            {"Release", dm::describeValue(eg, eg.r)}};
}

void forEachEGTimeCase(const std::function<void(const EGTime &, bool, float, float, bool)> &cb)
{
    for (const auto &eg : egTimes())
    {
        for (auto ts : {false, true})
        {
            if (ts && !eg.md.canTemposync)
                continue;
            for (auto base : {0.f, 0.05f, 0.2f, 0.5f, eg.md.defaultVal, 0.9f})
                for (auto depth : {0.1f, 0.5f, 1.f})
                    for (auto bip : {false, true})
                        cb(eg, ts, base, depth, bip);
        }
    }
}
} // namespace

TEST_CASE("Dump modulation tooltips", "[.][tooltips]")
{
    std::set<std::string> printed;
    scxt::tests::forEachProcessorState([&printed](const auto &s) {
        for (int f = 0; f < s.desc.numFloatParams; ++f)
        {
            const auto &md = s.desc.floatControlDescriptions[f];
            if (md.name.empty())
                continue;
            std::ostringstream key;
            key << pdsp::getProcessorName(s.type) << " kt=" << s.keytrack << " " << f << " "
                << md.name << " [" << md.minVal << "," << md.maxVal << "] " << md.unit << " "
                << (int)md.displayScale << " " << md.quantize(md.minVal + 0.37f);
            if (!printed.insert(key.str()).second)
                continue;
            std::cout << pdsp::getProcessorName(s.type) << " kt=" << s.keytrack << " (" << s.ints
                      << ") | " << md.name << " [" << md.minVal << ", " << md.maxVal << "]"
                      << std::endl;
            for (auto ts : {false, true})
            {
                if (ts && !md.canTemposync)
                    continue;
                auto fs = pmd::FeatureState{}.withTemposync(ts);
                for (auto b : baseValuesFor(md, s.floats[f]))
                    for (auto d : {0.1f, 0.5f, 1.f})
                        std::cout << "    " << (ts ? "ts " : "") << "base=" << b << " depth=" << d
                                  << " : " << tooltipLine(md, b, d, true, fs) << std::endl;
            }
        }
    });

    forEachEGTimeCase([](const auto &eg, auto ts, auto base, auto depth, auto bip) {
        if (!bip)
            return;
        std::cout << "EG " << eg.name << " ts=" << ts << " base=" << base << " depth=" << depth
                  << " : "
                  << tooltipLine(eg.md, base, depth, true, pmd::FeatureState{}.withTemposync(ts))
                  << std::endl;
    });
}

TEST_CASE("Modulation tooltips are well formed")
{
    SECTION("Processor params")
    {
        scxt::tests::forEachProcessorState([](const auto &s) {
            for (int f = 0; f < s.desc.numFloatParams; ++f)
            {
                const auto &md = s.desc.floatControlDescriptions[f];
                if (md.name.empty())
                    continue;
                INFO(pdsp::getProcessorName(s.type)
                     << " kt=" << s.keytrack << " " << s.ints << " '" << md.name << "'");
                INFO("stored " << s.floats[f] << " range " << md.minVal << " " << md.maxVal);
                REQUIRE(std::isfinite(s.floats[f]));
                REQUIRE(s.floats[f] >= md.minVal);
                REQUIRE(s.floats[f] <= md.maxVal);
                for (auto ts : {false, true})
                {
                    if (ts && !md.canTemposync)
                        continue;
                    auto fs = pmd::FeatureState{}.withTemposync(ts);
                    for (auto b : baseValuesFor(md, s.floats[f]))
                        for (auto d : {0.1f, 0.5f, 1.f})
                            for (auto bip : {false, true})
                                checkTooltip(md, b, d, bip, fs);
                }
            }
        });
    }

    SECTION("EG times")
    {
        forEachEGTimeCase([](const auto &eg, auto ts, auto base, auto depth, auto bip) {
            INFO("EG " << eg.name);
            checkTooltip(eg.md, base, depth, bip, pmd::FeatureState{}.withTemposync(ts));
        });
    }
}

TEST_CASE("Processor mod targets carry their temposync state", "[modulation]")
{
    std::unique_ptr<scxt::engine::Engine> eng(makeEngine());
    auto &part = *eng->getPatch()->getPart(0);
    part.addGroup();
    addBlankZoneToGroup(part, 0, 0, 127);
    auto &g = *part.getGroup(0);
    auto &z = *g.getZone(0);

    for (auto ts : {false, true})
    {
        INFO("temposync " << ts);
        z.processorStorage[0].isTemposynced = ts;
        g.processorStorage[0].isTemposynced = ts;

        scxt::voice::modulation::Matrix zm;
        zm.forUIMode = true;
        scxt::voice::modulation::MatrixEndpoints zep{nullptr};
        zep.bindTargetBaseValues(zm, z);
        REQUIRE(zm.activeTargetsToFeatureState.at(zep.processorTarget[0].fpT[0]).isTemposynced ==
                ts);

        scxt::modulation::GroupMatrix gm;
        gm.forUIMode = true;
        scxt::modulation::GroupMatrixEndpoints gep{nullptr};
        gep.bindTargetBaseValues(gm, g);
        REQUIRE(gm.activeTargetsToFeatureState.at(gep.processorTarget[0].fpT[0]).isTemposynced ==
                ts);
    }
}
