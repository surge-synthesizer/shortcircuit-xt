/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#ifndef SCXT_SRC_DSP_RESAMPLING_H
#define SCXT_SRC_DSP_RESAMPLING_H

#include <cstdint>

namespace scxt::dsp
{
static constexpr uint32_t FIRipol_M = 256;
static constexpr uint32_t FIRipol_M_bits = 8;
static constexpr uint32_t FIRipol_N = 16;
static constexpr uint32_t FIRipolI16_N = 16;
static constexpr uint32_t FIRoffset = 8;
} // namespace scxt::dsp
#endif // SCXT_SRC_DSP_RESAMPLING_H
