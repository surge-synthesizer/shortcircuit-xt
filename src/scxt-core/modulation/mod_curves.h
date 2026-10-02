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

#ifndef SCXT_SRC_SCXT_CORE_MODULATION_MOD_CURVES_H
#define SCXT_SRC_SCXT_CORE_MODULATION_MOD_CURVES_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <thread>
#include <mutex>
#include <functional>
#include <vector>
#include <cassert>
#include <string>
#include <utility>
#include <unordered_map>
#include "utils.h"

namespace scxt::modulation
{
struct ModulationCurves
{

    using CurveIdentifier = uint32_t;

    static std::vector<CurveIdentifier> allCurves;
    static std::unordered_map<CurveIdentifier, std::pair<std::string, std::string>> curveNames;
    static std::unordered_map<CurveIdentifier, std::function<float(float)>> curveImpls;

    static inline void initializeCurves()
    {
        static std::mutex mtx;
        std::lock_guard<std::mutex> g(mtx);

        if (!allCurves.empty())
            return;

        auto add = [](uint32_t tag, const std::string &cat, const std::string &nm,
                      std::function<float(float)> fn) {
            auto ci = CurveIdentifier{tag};
            assert(curveNames.find(ci) == curveNames.end());
            allCurves.push_back(ci);
            curveNames.insert_or_assign(ci, std::make_pair(cat, nm));
            curveImpls.insert_or_assign(ci, fn);
        };
        // change anything you want *except* the first argument
        // which is the streaming id. the menu is created with empty
        // cat first then the others in order
        add('x2  ', "", "x^2", [](auto x) { return x * x; });
        add('x3  ', "", "x^3", [](auto x) { return x * x * x; });
        add('unip', "", "(x+1)/2", [](auto x) { return (x + 1.f) / 2.f; });
        add('bip ', "", "2x - 1", [](auto x) { return x * 2.f - 1.f; });
        add('1-x ', "", "1 - x", [](auto x) { return 1.f - x; });
        add('absx', "Rectifiers", "|x|", [](auto x) { return std::fabs(x); });
        add('hwpo', "Rectifiers", "max(x,0)", [](auto x) { return std::max(x, 0.f); });
        add('hwne', "Rectifiers", "min(x,0)", [](auto x) { return std::min(x, 0.f); });
        add('uwpo', "Rectifiers", "max(x,1/2)", [](auto x) { return std::max(x, 0.5f); });
        add('uwne', "Rectifiers", "min(x,1/2)", [](auto x) { return std::min(x, 0.5f); });

        add('cmp0', "Comparators", "x > 0", [](auto x) { return x > 0.f ? 1.f : 0.f; });
        add('cmn0', "Comparators", "x < 0", [](auto x) { return x < 0.f ? 1.f : 0.f; });
        add('cmph', "Comparators", "x > 1/2", [](auto x) { return x > 0.5f ? 1.f : 0.f; });
        add('cmnh', "Comparators", "x < 1/2", [](auto x) { return x < 0.5f ? 1.f : 0.f; });

        add('sinx', "Waveforms", std::string("sin(2") + u8"\U000003C0" + "x)", // thats pi
            [](auto x) { return std::sin(2.0 * M_PI * x); });
        add('cosx', "Waveforms", std::string("cos(2") + u8"\U000003C0" + "x)", // thats pi
            [](auto x) { return std::cos(2.0 * M_PI * x); });
        add('trix', "Waveforms", std::string("tri(x)"), [](auto x) -> float {
            auto res = 0.f;
            if (x < 0)
                x += 1;
            if (x < 0.25)
            {
                // 0 -> 0.25 maps to 0 -> 1
                res = 4 * x;
            }
            else if (x < 0.75)
            {
                // 0.25 -> 0.75 maps to 1 -> -1
                res = (1.f - 4 * (x - 0.25));
            }
            else
            {
                // 0.75 -> 1 maps to -1 to 0
                res = (x - 1) * 4;
            }
            return res;
        });
        add('trip', "Waveforms", "tri(x+1/4)", [](auto x) -> float {
            auto res = 0.f;
            if (x < 0)
                x += 1;
            if (x < 0.5)
            {
                // 0 -> 0.5 maps to -1 -> 1
                res = 4 * x - 1.f;
            }
            else
            {
                // 0.5 -> 1 maps to 1 -> -1
                res = (1.f - 4 * (x - 0.5));
            }
            return res;
        });

        add('d.1 ', "Scale", "x / 10", [](auto x) { return x * 0.1; });
        add('d.01', "Scale", "x / 100", [](auto x) { return x * 0.01; });

        add('r01e', "Curve 01 remappers", "slow early rise",
            [](auto x) { return remap01(x, remapSlowBend, true); });
        add('r01E', "Curve 01 remappers", "fast early rise",
            [](auto x) { return remap01(x, remapFastBend, true); });
        add('r01l', "Curve 01 remappers", "slow late rise",
            [](auto x) { return remap01(x, remapSlowBend, false); });
        add('r01L', "Curve 01 remappers", "fast late rise",
            [](auto x) { return remap01(x, remapFastBend, false); });
        add('f01e', "Curve 01 remappers", "slow early fall",
            [](auto x) { return fall01(x, remapSlowBend, true); });
        add('f01E', "Curve 01 remappers", "fast early fall",
            [](auto x) { return fall01(x, remapFastBend, true); });
        add('f01l', "Curve 01 remappers", "slow late fall",
            [](auto x) { return fall01(x, remapSlowBend, false); });
        add('f01L', "Curve 01 remappers", "fast late fall",
            [](auto x) { return fall01(x, remapFastBend, false); });
    }

    // 10^-6/5 and 10^-12/5; fast late rise is the SF2 concave curve
    static constexpr float remapSlowBend{0.0630957344f}, remapFastBend{0.0039810717f};

    // log bend through (0,0) and (1,1), odd symmetric and saturating outside +/-1
    static float remap01(float x, float eps, bool early)
    {
        auto ax = std::min(std::fabs(x), 1.f);
        auto u = early ? 1.f - ax : ax;
        auto y = std::log(1.f - (1.f - eps) * u) / std::log(eps);
        return std::copysign(early ? 1.f - y : y, x);
    }

    // the rise mirrored, so (0,1) to (1,0); even in x and 0 outside +/-1
    static float fall01(float x, float eps, bool early)
    {
        return remap01(1.f - std::min(std::fabs(x), 1.f), eps, !early);
    }

    static std::function<float(float)> getCurveOperator(CurveIdentifier id)
    {
        auto ptr = curveImpls.find(id);
        assert(ptr != curveImpls.end());
        if (ptr == curveImpls.end())
            return nullptr;
        else
            return ptr->second;
    }
};
} // namespace scxt::modulation

#endif // SHORTCIRCUITXT_MOD_CURVES_H
