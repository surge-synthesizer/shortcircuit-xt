/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2024, Various authors, as described in the github
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

#ifndef SCXT_SRC_MODULATION_MOD_CURVES_H
#define SCXT_SRC_MODULATION_MOD_CURVES_H

#include <cmath>
#include <cstdint>
#include <thread>
#include <mutex>
#include <functional>
#include <vector>
#include <cassert>
#include <unordered_map>

namespace scxt::modulation
{
struct ModulationCurves
{

    using CurveIdentifier = uint32_t;

    static std::vector<CurveIdentifier> allCurves;
    static std::unordered_map<CurveIdentifier, std::string> curveNames;
    static std::unordered_map<CurveIdentifier, std::function<float(float)>> curveImpls;

    static inline void initializeCurves()
    {
        static std::mutex mtx;
        std::lock_guard<std::mutex> g(mtx);

        if (!allCurves.empty())
            return;

        auto add = [](uint32_t tag, const std::string &nm, std::function<float(float)> fn) {
            auto ci = CurveIdentifier{tag};
            assert(curveNames.find(ci) == curveNames.end());
            allCurves.push_back(ci);
            curveNames.insert_or_assign(ci, nm);
            curveImpls.insert_or_assign(ci, fn);
        };
        add('x3  ', "x^3", [](auto x) { return x * x * x; });
        add('absx', "|x|", [](auto x) { return std::fabs(x); });
        add('unip', "(x+1)/2)", [](auto x) { return (x + 1.f) / 2.f; });
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
