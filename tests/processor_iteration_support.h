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

#ifndef SCXT_TESTS_PROCESSOR_ITERATION_SUPPORT_H
#define SCXT_TESTS_PROCESSOR_ITERATION_SUPPORT_H

#include <algorithm>
#include <functional>
#include <string>

#include "catch2/catch2.hpp"
#include "dsp/processor/processor.h"
#include "engine/memory_pool.h"
#include "engine/engine.h"

namespace scxt::tests
{
struct ProcessorIterationState
{
    dsp::processor::ProcessorType type;
    bool keytrack;
    std::string ints; // "defaults" or "name=value"
    const dsp::processor::ProcessorControlDescription &desc;
    const float *floats;
    const int *intValues;
};

// spawn each processor in each keytrack state and every value of each int param in turn,
// remapping and clamping the floats the way the engine does on an int change
inline void forEachProcessorState(const std::function<void(const ProcessorIterationState &)> &cb)
{
    namespace pdsp = dsp::processor;
    engine::Engine e;
    engine::MemoryPool mp;

    for (int i = 1; i < pdsp::proct_num_types; ++i)
    {
        auto pt = (pdsp::ProcessorType)i;
        if (!pdsp::isProcessorImplemented(pt))
            continue;

        for (auto kt : {false, true})
        {
            alignas(16) uint8_t memory[pdsp::processorMemoryBufferSize]{};
            float pfp[pdsp::maxProcessorFloatParams]{};
            int ifp[pdsp::maxProcessorIntParams]{};
            pdsp::ProcessorStorage ps;
            ps.type = pt;
            ps.isKeytracked = kt;

            auto p = pdsp::spawnProcessorInPlace(
                pt, &e, &mp, memory, pdsp::processorMemoryBufferSize, ps, pfp, ifp, false, true);
            REQUIRE(p);
            p->setKeytrack(kt);
            p->init_params();
            p->resetMetadata();

            auto desc = p->getControlDescription();
            if (kt && !desc.supportsKeytrack)
            {
                pdsp::unspawnProcessor(p);
                continue;
            }
            cb({pt, kt, "defaults", desc, pfp, ifp});

            float defF[pdsp::maxProcessorFloatParams];
            std::copy(pfp, pfp + pdsp::maxProcessorFloatParams, defF);
            for (int ip = 0; ip < desc.numIntParams; ++ip)
            {
                const auto &imd = desc.intControlDescriptions[ip];
                auto def = ifp[ip];
                auto mx = std::min((int)imd.maxVal, (int)imd.minVal + 32);
                for (int v = (int)imd.minVal; v <= mx; ++v)
                {
                    if (v == def)
                        continue;
                    ifp[ip] = v;
                    p->remapFloatsForIntChange(ip, def, pfp);
                    p->resetMetadata();
                    auto vd = p->getControlDescription();
                    for (int f = 0; f < vd.numFloatParams; ++f)
                    {
                        const auto &md = vd.floatControlDescriptions[f];
                        pfp[f] = std::clamp(pfp[f], md.minVal, md.maxVal);
                    }
                    cb({pt, kt, imd.name + "=" + std::to_string(v), vd, pfp, ifp});
                    std::copy(defF, defF + pdsp::maxProcessorFloatParams, pfp);
                }
                ifp[ip] = def;
            }
            p->resetMetadata();
            pdsp::unspawnProcessor(p);
        }
    }
}
} // namespace scxt::tests

#endif // SCXT_TESTS_PROCESSOR_ITERATION_SUPPORT_H
