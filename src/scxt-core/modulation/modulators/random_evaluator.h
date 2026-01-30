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

#ifndef SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_RANDOM_EVALUATOR_H
#define SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_RANDOM_EVALUATOR_H

#include <array>
#include "configuration.h"
#include "sst/basic-blocks/dsp/RNG.h"
#include "modulation/modulator_storage.h"

namespace scxt::modulation::modulators
{
struct RandomEvaluator
{
    sst::basic_blocks::dsp::RNG &rng;

    RandomEvaluator(sst::basic_blocks::dsp::RNG &extrng) : rng(extrng) {}

    std::array<float, scxt::randomsPerGroupOrZone> outputs;
    void evaluate(const modulation::MiscSourceStorage &rs)
    {
        for (int i = 0; i < scxt::randomsPerGroupOrZone; i++)
        {
            switch (rs.randoms[i].style)
            {
            case modulation::modulators::RandomStorage::UNIFORM_01:
                outputs[i] = rng.unif01();
                break;
            case modulation::modulators::RandomStorage::UNIFORM_BIPOLAR:
                outputs[i] = rng.unifPM1();
                break;
            case modulation::modulators::RandomStorage::NORMAL:
                outputs[i] = rng.normPM1();
                break;
            case modulation::modulators::RandomStorage::HALF_NORMAL:
                outputs[i] = rng.half01();
                break;
            }
        }
    }
};
} // namespace scxt::modulation::modulators
#endif
