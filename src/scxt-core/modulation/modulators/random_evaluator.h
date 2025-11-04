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

#ifndef SCXT_SRC_MODULATION_MODULATORS_RANDOM_EVALUATOR_H
#define SCXT_SRC_MODULATION_MODULATORS_RANDOM_EVALUATOR_H

#include <array>
#include "configuration.h"
#include "sst/basic-blocks/dsp/RNG.h"
#include "modulation/modulator_storage.h"

namespace scxt::modulation::modulators
{
struct RandomEvaluator
{
    std::array<float, scxt::randomsPerGroupOrZone> outputs;
    void evaluate(sst::basic_blocks::dsp::RNG &rng, const modulation::MiscSourceStorage &rs)
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
