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

#ifndef SCXT_SRC_SCXT_CORE_DSP_DATA_TABLES_H
#define SCXT_SRC_SCXT_CORE_DSP_DATA_TABLES_H

#include <cctype>
#include <cstdint>
#include <stddef.h> // for size_t on some linuxes it seems

#include "sst/basic-blocks/tables/SincTableProvider.h"
#include "sst/basic-blocks/tables/DbToLinearProvider.h"
#include "sst/basic-blocks/tables/TwoToTheXProvider.h"
#include "sst/basic-blocks/tables/SixSinesWaveProvider.h"
#include "sst/basic-blocks/tables/SimpleSineProvider.h"
#include "sst/basic-blocks/tables/ExpTimeProvider.h"
#include "resampling.h"

namespace scxt::dsp
{
using SincTable = sst::basic_blocks::tables::ShortcircuitSincTableProvider;
extern SincTable sincTable;

using SurgeSincTable = sst::basic_blocks::tables::SurgeSincTableProvider;
extern SurgeSincTable surgeSincTable;

static_assert(dsp::FIRipol_M == SincTable::FIRipol_M);
static_assert(dsp::FIRipol_N == SincTable::FIRipol_N);
static_assert(dsp::FIRipolI16_N == SincTable::FIRipolI16_N);

using DbTable = sst::basic_blocks::tables::DbToLinearProvider;
extern DbTable dbTable;

using TwoToTheXTable = sst::basic_blocks::tables::TwoToTheXProvider;
extern TwoToTheXTable twoToTheXTable;

using PmSineTable = sst::basic_blocks::tables::SixSinesWaveProvider;
extern PmSineTable pmSineTable;

// for the voice flanger LFO
using SimpleSineTable = sst::basic_blocks::tables::SimpleSineProvider;
extern SimpleSineTable simpleSineTable;

using TwentyFiveSecondExpTable = sst::basic_blocks::tables::TwentyFiveSecondExpTable;
extern TwentyFiveSecondExpTable twentyFiveSecondExpTable;
} // namespace scxt::dsp

#endif // SCXT_SRC_SCXT_CORE_DSP_DATA_TABLES_H
