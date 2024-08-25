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

#ifndef SCXT_SRC_MODULATION_HAS_MODULATORS_H
#define SCXT_SRC_MODULATION_HAS_MODULATORS_H

#include "configuration.h"

#include "sst/basic-blocks/modulators/ADSREnvelope.h"
#include "sst/basic-blocks/modulators/AHDSRShapedSC.h"
#include "modulation/modulators/steplfo.h"
#include "modulation/modulators/curvelfo.h"
#include "modulation/modulators/envlfo.h"

namespace scxt::modulation::shared
{

static_assert(lfosPerGroup == lfosPerZone,
              "If this is false you need to template out the count below");
static_assert(egsPerGroup == egsPerZone,
              "If this is false you need to template out the count below");
template <typename T> struct HasModulators
{
    HasModulators(T *that) : eg{that, that}, egOS{that, that} {}

    static constexpr uint16_t lfosPerObject{lfosPerZone};
    static constexpr uint16_t egsPerObject{egsPerZone};

    enum LFOEvaluator
    {
        STEP,
        CURVE,
        ENV,
        MSEG
    } lfoEvaluator[lfosPerObject]{};
    scxt::modulation::modulators::StepLFO stepLfos[lfosPerObject];
    scxt::modulation::modulators::CurveLFO curveLfos[lfosPerObject];
    scxt::modulation::modulators::EnvLFO envLfos[lfosPerObject];

    typedef sst::basic_blocks::modulators::AHDSRShapedSC<
        T, blockSize, sst::basic_blocks::modulators::TwentyFiveSecondExp>
        ahdsrenv_t;

    typedef sst::basic_blocks::modulators::AHDSRShapedSC<
        T, blockSize << 1, sst::basic_blocks::modulators::TwentyFiveSecondExp>
        ahdsrenvOS_t;

    ahdsrenv_t eg[egsPerObject];
    ahdsrenvOS_t egOS[egsPerObject];

    std::array<bool, lfosPerObject> lfosActive{};
    std::array<bool, egsPerObject> egsActive{};
};
} // namespace scxt::modulation::shared
#endif // SHORTCIRCUITXT_HAS_MODULATORS_H
