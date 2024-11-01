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

#include "sst/basic-blocks/simd/setup.h"

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
#include "sst/voice-effects/distortion/Slewer.h"
#include "sst/voice-effects/distortion/TreeMonster.h"
#include "sst/voice-effects/waveshaper/WaveShaper.h"

#include "sst/voice-effects/eq/EqNBandParametric.h"
#include "sst/voice-effects/eq/EqGraphic6Band.h"
#include "sst/voice-effects/eq/MorphEQ.h"
#include "sst/voice-effects/eq/TiltEQ.h"

#include "sst/voice-effects/filter/CytomicSVF.h"
#include "sst/voice-effects/filter/SSTFilters.h"
#include "sst/voice-effects/filter/StaticPhaser.h"

#include "sst/voice-effects/generator/GenCorrelatedNoise.h"
#include "sst/voice-effects/generator/TiltNoise.h"
#include "sst/voice-effects/generator/GenVA.h"
#include "sst/voice-effects/delay/StringResonator.h"

#include "sst/voice-effects/modulation/FreqShiftMod.h"
#include "sst/voice-effects/modulation/RingMod.h"
#include "sst/voice-effects/modulation/PhaseMod.h"
#include "sst/voice-effects/modulation/FMFilter.h"
#include "sst/voice-effects/modulation/Tremolo.h"
#include "sst/voice-effects/modulation/Phaser.h"
#include "sst/voice-effects/modulation/ShepardPhaser.h"
#include "sst/voice-effects/modulation/NoiseAM.h"
#include "sst/voice-effects/delay/Chorus.h"
#include "sst/voice-effects/utilities/VolumeAndPan.h"
#include "sst/voice-effects/utilities/StereoTool.h"
#include "sst/voice-effects/utilities/GainMatrix.h"
#include "sst/voice-effects/dynamics/Compressor.h"
#include "sst/voice-effects/dynamics/AutoWah.h"

#include "sst/voice-effects/lifted_bus_effects/LiftedReverb1.h"
#include "sst/voice-effects/lifted_bus_effects/LiftedReverb2.h"
#include "sst/voice-effects/lifted_bus_effects/LiftedDelay.h"
#include "sst/voice-effects/lifted_bus_effects/LiftedFlanger.h"

namespace scxt::dsp::processor
{
// Just don't change the id or streaming name, basically

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
DEFINE_PROC(Slewer, sst::voice_effects::distortion::Slewer<SCXTVFXConfig<1>>,
            sst::voice_effects::distortion::Slewer<SCXTVFXConfig<2>>, proct_fx_slewer, "Slewer",
            "Distortion", "slewer-fx");
DEFINE_PROC(TreeMonster, sst::voice_effects::distortion::TreeMonster<SCXTVFXConfig<1>>,
            sst::voice_effects::distortion::TreeMonster<SCXTVFXConfig<2>>, proct_fx_treemonster,
            "Treemonster", "Distortion", "treemonster-voice");

DEFINE_PROC(Compressor, sst::voice_effects::dynamics::Compressor<SCXTVFXConfig<1>>,
            sst::voice_effects::dynamics::Compressor<SCXTVFXConfig<2>>, proct_Compressor,
            "Compressor", "Dynamics", "compressor");
DEFINE_PROC(AutoWah, sst::voice_effects::dynamics::AutoWah<SCXTVFXConfig<1>>,
            sst::voice_effects::dynamics::AutoWah<SCXTVFXConfig<2>>, proct_autowah, "Auto Wah",
            "Dynamics", "autowah");

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

DEFINE_PROC(TiltEQ, sst::voice_effects::eq::TiltEQ<SCXTVFXConfig<1>>,
            sst::voice_effects::eq::TiltEQ<SCXTVFXConfig<2>>, proct_eq_tilt, "Tilt EQ", "EQ",
            "eq-tilt");
DEFINE_PROC(VolPan, sst::voice_effects::utilities::VolumeAndPan<SCXTVFXConfig<1>>,
            sst::voice_effects::utilities::VolumeAndPan<SCXTVFXConfig<2>>, proct_volpan,
            "Volume & Pan", "Utility", "volume-pan");
DEFINE_PROC(StereoTool, sst::voice_effects::utilities::StereoTool<SCXTVFXConfig<1>>,
            sst::voice_effects::utilities::StereoTool<SCXTVFXConfig<2>>, proct_stereotool,
            "Stereo Tool", "Utility", "stereo-tool");
DEFINE_PROC(GainMatrix, sst::voice_effects::utilities::GainMatrix<SCXTVFXConfig<1>>,
            sst::voice_effects::utilities::GainMatrix<SCXTVFXConfig<2>>, proct_gainmatrix,
            "Gain Matrix", "Utility", "gain-matrix");
DEFINE_PROC(Widener, sst::voice_effects::delay::Widener<SCXTVFXConfig<1>>,
            sst::voice_effects::delay::Widener<SCXTVFXConfig<2>>, proct_fx_widener, "Widener",
            "Utility", "fxstereo-fx", dsp::surgeSincTable);

DEFINE_PROC(GenVA, sst::voice_effects::generator::GenVA<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::GenVA<SCXTVFXConfig<2>>, proct_osc_VA, "VA Oscillator",
            "Generators", "osc-va", dsp::sincTable);
PROC_DEFAULT_MIX(proct_osc_VA, 0.5);

DEFINE_PROC(GenCorrelatedNoise, sst::voice_effects::generator::GenCorrelatedNoise<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::GenCorrelatedNoise<SCXTVFXConfig<2>>,
            proct_osc_correlatednoise, "Correlated Noise", "Generators", "osc-correlated-noise");
PROC_DEFAULT_MIX(proct_osc_correlatednoise, 0.5);

DEFINE_PROC(GenTiltNoise, sst::voice_effects::generator::TiltNoise<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::TiltNoise<SCXTVFXConfig<2>>, proct_osc_tiltnoise,
            "Tilt Noise", "Generators", "osc-tilt-noise");
PROC_DEFAULT_MIX(proct_osc_tiltnoise, 0.5);

DEFINE_PROC(StringResonator, sst::voice_effects::delay::StringResonator<SCXTVFXConfig<1>>,
            sst::voice_effects::delay::StringResonator<SCXTVFXConfig<2>>, proct_stringResonator,
            "String Resonator", "Generators", "stringex-fx", dsp::surgeSincTable);

DEFINE_PROC(MorphEQ, sst::voice_effects::eq::MorphEQ<SCXTVFXConfig<1>>,
            sst::voice_effects::eq::MorphEQ<SCXTVFXConfig<2>>, proct_eq_morph, "Morph", "Filters",
            "eq-morph");
DEFINE_PROC(CytomicSVF, sst::voice_effects::filter::CytomicSVF<SCXTVFXConfig<1>>,
            sst::voice_effects::filter::CytomicSVF<SCXTVFXConfig<2>>, proct_CytomicSVF, "Fast SVF",
            "Filters", "filt-cytomic");
DEFINE_PROC(SSTFilters, sst::voice_effects::filter::SSTFilters<SCXTVFXConfig<1>>,
            sst::voice_effects::filter::SSTFilters<SCXTVFXConfig<2>>, proct_SurgeFilters,
            "Surge Filters", "Filters", "filt-sstfilters");
DEFINE_PROC(StaticPhaser, sst::voice_effects::filter::StaticPhaser<SCXTVFXConfig<1>>,
            sst::voice_effects::filter::StaticPhaser<SCXTVFXConfig<2>>, proct_StaticPhaser,
            "Static Phaser", "Filters", "filt-statph");

DEFINE_PROC(FreqShiftMod, sst::voice_effects::modulation::FreqShiftMod<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::FreqShiftMod<SCXTVFXConfig<2>>, proct_fx_freqshiftmod,
            "Freqshift Mod", "Audio Rate Mod", "pitchring-fx");
DEFINE_PROC(PhaseMod, sst::voice_effects::modulation::PhaseMod<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::PhaseMod<SCXTVFXConfig<2>>, proct_osc_phasemod,
            "Phase Mod", "Audio Rate Mod", "osc-phase-mod");
DEFINE_PROC(FMFilter, sst::voice_effects::modulation::FMFilter<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::FMFilter<SCXTVFXConfig<2>>, proct_fmfilter, "FM Filter",
            "Audio Rate Mod", "filt-fm");
DEFINE_PROC(RingMod, sst::voice_effects::modulation::RingMod<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::RingMod<SCXTVFXConfig<2>>, proct_fx_ringmod, "Ring Mod",
            "Audio Rate Mod", "ringmod-fx");
DEFINE_PROC(NoiseAM, sst::voice_effects::modulation::NoiseAM<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::NoiseAM<SCXTVFXConfig<2>>, proct_noise_am, "Noise AM",
            "Audio Rate Mod", "noise-am");

DEFINE_PROC(Tremolo, sst::voice_effects::modulation::Tremolo<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::Tremolo<SCXTVFXConfig<2>>, proct_Tremolo, "Tremolo",
            "Modulation", "tremolo");
DEFINE_PROC(Phaser, sst::voice_effects::modulation::Phaser<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::Phaser<SCXTVFXConfig<2>>, proct_Phaser, "Phaser",
            "Modulation", "modulated-phaser");
DEFINE_PROC(ShepardPhaser, sst::voice_effects::modulation::ShepardPhaser<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::ShepardPhaser<SCXTVFXConfig<2>>, proct_shepard,
            "Shepard Phaser", "Modulation", "shepard");
DEFINE_PROC(Chorus, sst::voice_effects::delay::Chorus<SCXTVFXConfig<1>>,
            sst::voice_effects::delay::Chorus<SCXTVFXConfig<2>>, proct_Chorus, "Chorus",
            "Modulation", "voice-chorus", dsp::surgeSincTable);

DEFINE_PROC(LiftedReverb1, sst::voice_effects::liftbus::LiftedReverb1<SCXTVFXConfig<1>>,
            sst::voice_effects::liftbus::LiftedReverb1<SCXTVFXConfig<2>>, proct_lifted_reverb1,
            "Reverb1", "Reverb", "lifted-reverb1");
PROC_FOR_GROUP_ONLY(proct_lifted_reverb1);
PROC_DEFAULT_MIX(proct_lifted_reverb1, 0.333);

DEFINE_PROC(LiftedReverb2, sst::voice_effects::liftbus::LiftedReverb2<SCXTVFXConfig<1>>,
            sst::voice_effects::liftbus::LiftedReverb2<SCXTVFXConfig<2>>, proct_lifted_reverb2,
            "Reverb2", "Reverb", "lifted-reverb2");
PROC_FOR_GROUP_ONLY(proct_lifted_reverb2);
PROC_DEFAULT_MIX(proct_lifted_reverb2, 0.333);

DEFINE_PROC(LiftedDelay, sst::voice_effects::liftbus::LiftedDelay<SCXTVFXConfig<1>>,
            sst::voice_effects::liftbus::LiftedDelay<SCXTVFXConfig<2>>, proct_lifted_delay,
            "Dual Delay", "Delay", "lifted-delay");
PROC_FOR_GROUP_ONLY(proct_lifted_delay);
PROC_DEFAULT_MIX(proct_lifted_delay, 0.333);

DEFINE_PROC(LiftedFlanger, sst::voice_effects::liftbus::LiftedFlanger<SCXTVFXConfig<1>>,
            sst::voice_effects::liftbus::LiftedFlanger<SCXTVFXConfig<2>>, proct_lifted_flanger,
            "Flanger", "Modulation", "lifted-flanger");
PROC_FOR_GROUP_ONLY(proct_lifted_flanger);

} // namespace scxt::dsp::processor

#endif // __SCXT_PROCESSOR_DEFS_H
