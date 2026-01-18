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