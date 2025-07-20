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

#include "catch2/catch2.hpp"
#include "dsp/processor/processor.h"

TEST_CASE("Processors have Formatting")
{
    namespace pdsp = scxt::dsp::processor;
    scxt::engine::MemoryPool mp;
    pdsp::ProcessorStorage procStorage;
    uint8_t memory[dsp::processor::processorMemoryBufferSize];
    float pfp[dsp::processor::maxProcessorFloatParams];
    int ifp[dsp::processor::maxProcessorIntParams];

    // Akip peoxr_nonw
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
                auto p =
                    pdsp::spawnProcessorInPlace(pt, &mp, memory, pdsp::processorMemoryBufferSize,
                                                procStorage, pfp, ifp, false, true);
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