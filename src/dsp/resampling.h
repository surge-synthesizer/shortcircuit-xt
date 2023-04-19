/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#ifndef SCXT_SRC_DSP_RESAMPLING_H
#define SCXT_SRC_DSP_RESAMPLING_H

#include <cstdint>
#include "sst/basic-blocks/tables/SincTableProvider.h"

namespace scxt::dsp
{
static constexpr uint32_t FIRipol_M = 256;
static constexpr uint32_t FIRipol_N = 16;
static constexpr uint32_t FIRipolI16_N = 16;
static constexpr uint32_t FIRoffset = 8;
} // namespace scxt::dsp
#endif // SCXT_SRC_DSP_RESAMPLING_H
