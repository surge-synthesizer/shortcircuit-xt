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
#include <map>

#include "catch2/catch2.hpp"
#include "processor_iteration_support.h"

namespace pdsp = scxt::dsp::processor;

TEST_CASE("Processor float param names are distinct and valid")
{
    scxt::tests::forEachProcessorState([](const auto &s) {
        std::map<std::string, int> seen;
        for (int f = 0; f < s.desc.numFloatParams; ++f)
        {
            const auto &md = s.desc.floatControlDescriptions[f];
            INFO(pdsp::getProcessorName(s.type) << " keytrack=" << s.keytrack << " " << s.ints
                                                << " param " << f << " '" << md.name << "'");
            for (const auto &n : {md.name, md.shortName})
            {
                REQUIRE(n != "Error");
                REQUIRE(n.rfind("Unknown", 0) != 0);
            }
            if (md.name.empty())
                continue;
            auto [it, inserted] = seen.emplace(md.name, f);
            INFO("collides with param " << it->second);
            REQUIRE(inserted);
        }
    });
}

TEST_CASE("Dump processor param names", "[.][param-names]")
{
    scxt::tests::forEachProcessorState([](const auto &s) {
        std::string ints;
        for (int i = 0; i < s.desc.numIntParams; ++i)
            ints += (i ? "," : "") + std::to_string(s.intValues[i]);
        for (int f = 0; f < s.desc.numFloatParams; ++f)
        {
            const auto &md = s.desc.floatControlDescriptions[f];
            std::cout << pdsp::getProcessorName(s.type) << "\t" << s.keytrack << "\t" << ints
                      << "\t" << f << "\t" << md.name << "\t" << md.shortName << std::endl;
        }
    });
}
