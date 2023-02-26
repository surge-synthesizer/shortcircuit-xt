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

#ifndef SCXT_SRC_DSP_DATA_TABLES_H
#define SCXT_SRC_DSP_DATA_TABLES_H

#include "resampling.h"
#include <cctype>
#include <cstdint>
#include <stddef.h> // for size_t on some linuxes it seems

namespace scxt::dsp
{
struct SincTable
{
    void init();

    // TODO Rename these when i'm all done
    float SincTableF32[(FIRipol_M + 1) * FIRipol_N];
    float SincOffsetF32[(FIRipol_M)*FIRipol_N];
    short SincTableI16[(FIRipol_M + 1) * FIRipol_N];
    short SincOffsetI16[(FIRipol_M)*FIRipol_N];

  private:
    bool initialized{false};
};

extern SincTable sincTable;

struct DbTable
{
    static constexpr size_t nPoints{512};
    static_assert(!(nPoints & (nPoints - 1)));

    void init();
    float dbToLinear(float db);

  private:
    float table_dB[nPoints];
};

extern DbTable dbTable;
} // namespace scxt::dsp

#endif // __SCXT_DSP_SINC_TABLES_H
