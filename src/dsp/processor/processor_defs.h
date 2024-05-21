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

#include "definition_helpers.h"
#include "dsp/processor/processor_impl.h"

#include "sst/voice-effects/delay/Widener.h"
#include "sst/voice-effects/delay/ShortDelay.h"
#include "sst/voice-effects/delay/Microgate.h"

#include "sst/voice-effects/distortion/BitCrusher.h"
#include "sst/voice-effects/waveshaper/WaveShaper.h"

#include "sst/voice-effects/eq/EqNBandParametric.h"
#include "sst/voice-effects/eq/EqGraphic6Band.h"
#include "sst/voice-effects/eq/MorphEQ.h"

#include "sst/voice-effects/filter/CytomicSVF.h"
// #include "sst/voice-effects/filter/SurgeBiquads.h"
#include "sst/voice-effects/filter/SSTFilters.h"
#include "sst/voice-effects/filter/Slewer.h"

#include "sst/voice-effects/generator/GenCorrelatedNoise.h"
#include "sst/voice-effects/generator/GenSin.h"
#include "sst/voice-effects/generator/GenSaw.h"
#include "sst/voice-effects/generator/GenPulseSync.h"
#include "sst/voice-effects/delay/StringResonator.h"

#include "sst/voice-effects/modulation/FreqShiftMod.h"
#include "sst/voice-effects/modulation/RingMod.h"
#include "sst/voice-effects/modulation/PhaseMod.h"
#include "sst/voice-effects/modulation/StaticPhaser.h"

namespace scxt::dsp::processor
{
// Just don't change the id or streaming name, basically
DEFINE_PROC(Widener, sst::voice_effects::delay::Widener<SCXTVFXConfig<1>>,
            sst::voice_effects::delay::Widener<SCXTVFXConfig<2>>, proct_fx_widener, "Widener",
            "Delay", "fxstereo-fx", dsp::surgeSincTable);
DEFINE_PROC(ShortDelay, sst::voice_effects::delay::ShortDelay<SCXTVFXConfig<1>>,
            sst::voice_effects::delay::ShortDelay<SCXTVFXConfig<2>>, proct_fx_simple_delay,
            "Simple Delay", "Delay", "simpdel-fx", dsp::surgeSincTable);
PROC_DEFAULT_MIX(proct_fx_simple_delay, 0.3);

DEFINE_PROC(MicroGate, sst::voice_effects::delay::MicroGate<SCXTVFXConfig<1>>,
            sst::voice_effects::delay::MicroGate<SCXTVFXConfig<2>>, proct_fx_microgate, "MicroGate",
            "Delay", "micro-gate-fx");

DEFINE_PROC(BitCrusher, sst::voice_effects::distortion::BitCrusher<SCXTVFXConfig<1>>,
            sst::voice_effects::distortion::BitCrusher<SCXTVFXConfig<2>>, proct_fx_bitcrusher,
            "BitCrusher", "Distortion", "bit-crusher-fx");
DEFINE_PROC(WaveShaper, sst::voice_effects::waveshaper::WaveShaper<SCXTVFXConfig<1>>,
            sst::voice_effects::waveshaper::WaveShaper<SCXTVFXConfig<2>>, proct_fx_waveshaper,
            "WaveShaper", "Distortion", "waveshaper-fx");

// Macros and commas don't get along
namespace procimpl::detail
{
using eq3impl = sst::voice_effects::eq::EqNBandParametric<SCXTVFXConfig<1>, 3>;
using eq3impl_os = sst::voice_effects::eq::EqNBandParametric<SCXTVFXConfig<2>, 3>;

} // namespace procimpl::detail

DEFINE_PROC(EQ3Band, procimpl::detail::eq3impl, procimpl::detail::eq3impl_os,
            proct_eq_3band_parametric_A, "3 Band Parametric", "EQ", "eq-parm-3band");
DEFINE_PROC(EQGraphic6Band, sst::voice_effects::eq::EqGraphic6Band<SCXTVFXConfig<1>>,
            sst::voice_effects::eq::EqGraphic6Band<SCXTVFXConfig<2>>, proct_eq_6band,
            "6 Band Graphic", "EQ", "eq-grp-6");
DEFINE_PROC(MorphEQ, sst::voice_effects::eq::MorphEQ<SCXTVFXConfig<1>>,
            sst::voice_effects::eq::MorphEQ<SCXTVFXConfig<2>>, proct_eq_morph, "Morph", "EQ",
            "eq-morph");

DEFINE_PROC(GenSin, sst::voice_effects::generator::GenSin<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::GenSin<SCXTVFXConfig<2>>, proct_osc_sin, "Sin",
            "Generators", "osc-sin");
PROC_DEFAULT_MIX(proct_osc_sin, 0.5);

DEFINE_PROC(GenSaw, sst::voice_effects::generator::GenSaw<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::GenSaw<SCXTVFXConfig<2>>, proct_osc_saw, "Saw",
            "Generators", "osc-saw");
PROC_DEFAULT_MIX(proct_osc_saw, 0.5);

DEFINE_PROC(GenPulseSync, sst::voice_effects::generator::GenPulseSync<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::GenPulseSync<SCXTVFXConfig<2>>, proct_osc_pulse_sync,
            "Pulse Sync", "Generators", "osc-pulse-sync", dsp::sincTable);
PROC_DEFAULT_MIX(proct_osc_pulse_sync, 0.5);

DEFINE_PROC(GenCorrelatedNoise, sst::voice_effects::generator::GenCorrelatedNoise<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::GenCorrelatedNoise<SCXTVFXConfig<2>>,
            proct_osc_correlatednoise, "Correlated Noise", "Generators", "osc-correlated-noise");
PROC_DEFAULT_MIX(proct_osc_correlatednoise, 0.5);

DEFINE_PROC(StringResonator, sst::voice_effects::delay::StringResonator<SCXTVFXConfig<1>>,
            sst::voice_effects::delay::StringResonator<SCXTVFXConfig<2>>, proct_stringResonator,
            "String Resonator", "Generators", "stringex-fx", dsp::surgeSincTable);
PROC_DEFAULT_MIX(proct_stringResonator, 0.5);

DEFINE_PROC(CytomicSVF, sst::voice_effects::filter::CytomicSVF<SCXTVFXConfig<1>>,
            sst::voice_effects::filter::CytomicSVF<SCXTVFXConfig<2>>, proct_CytomicSVF, "Fast SVF",
            "Filters", "filt-cytomic");
/* DEFINE_PROC(SurgeBiquads, sst::voice_effects::filter::SurgeBiquads<SCXTVFXConfig<1>>,
            sst::voice_effects::filter::SurgeBiquads<SCXTVFXConfig<2>>, proct_SurgeBiquads,
            "Surge Biquads", "Filters", "filt-sstbiquad"); */
DEFINE_PROC(SSTFilters, sst::voice_effects::filter::SSTFilters<SCXTVFXConfig<1>>,
            sst::voice_effects::filter::SSTFilters<SCXTVFXConfig<2>>, proct_SurgeFilters,
            "Surge Filters", "Filters", "filt-sstfilters");
DEFINE_PROC(Slewer, sst::voice_effects::filter::Slewer<SCXTVFXConfig<1>>,
            sst::voice_effects::filter::Slewer<SCXTVFXConfig<2>>, proct_fx_slewer, "Slewer",
            "Filters", "slewer-fx");

DEFINE_PROC(FreqShiftMod, sst::voice_effects::modulation::FreqShiftMod<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::FreqShiftMod<SCXTVFXConfig<2>>, proct_fx_freqshiftmod,
            "Freqshift Mod", "Modulation", "pitchring-fx");
DEFINE_PROC(PhaseMod, sst::voice_effects::modulation::PhaseMod<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::PhaseMod<SCXTVFXConfig<2>>, proct_osc_phasemod,
            "Phase Mod", "Modulation", "osc-phase-mod");
DEFINE_PROC(RingMod, sst::voice_effects::modulation::RingMod<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::RingMod<SCXTVFXConfig<2>>, proct_fx_ringmod, "Ring Mod",
            "Modulation", "ringmod-fx");
DEFINE_PROC(StaticPhaser, sst::voice_effects::modulation::StaticPhaser<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::StaticPhaser<SCXTVFXConfig<2>>, proct_StaticPhaser,
            "Static Phaser", "Modulation", "filt-statph");

} // namespace scxt::dsp::processor

#endif // __SCXT_PROCESSOR_DEFS_H
