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

#ifndef SCXT_SRC_DSP_PROCESSOR_PROCESSOR_DEFS_H
#define SCXT_SRC_DSP_PROCESSOR_PROCESSOR_DEFS_H

#include "processor.h"
#include "datamodel/metadata.h"
#include "infrastructure/sse_include.h"

#include <sst/filters/HalfRateFilter.h>
#include "vembertech/vt_dsp/lipol.h"
#include "sst/basic-blocks/dsp/BlockInterpolators.h"
#include "configuration.h"

/**
 * Adding a processor here. Used to be we had a bunch of case staements. But now we are all
 * driven by templates (see "processor.cpp" for some hairy template code you never need to touch).
 * So to add a processor
 *
 * 1. Add it in a .c/.hpp as a subtype of Processor
 * 2. Implement these constexpr values
 *       static constexpr const char *processorName{"OSC Pulse"};
 *       static constexpr const char *processorStreamingName{"osc-pulse"};
 *       static constexpr const char *processorDisplayGroup{"Generators"};
 *
 *       The streaming name has to be stable across versions
 *
 * 3. Specialize the id-to-type structure
 *      template <> struct ProcessorImplementor<ProcessorType::proct_osc_pulse_sync>
 *      {
 *          typedef OscPulseSync T;
 *      };
 *
 *  and then include the h here and everything will work
 */

#include "definitions/distortion_defs.h"
#include "definitions/eq_defs.h"
#include "definitions/generator_defs.h"
#include "definitions/pitch_defs.h"
#include "definitions/waveshaper_defs.h"

// port

#include "filter/supersvf.h"

#endif // __SCXT_PROCESSOR_DEFS_H
