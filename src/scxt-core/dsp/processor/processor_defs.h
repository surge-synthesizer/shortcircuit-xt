/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_CORE_DSP_PROCESSOR_PROCESSOR_DEFS_H
#define SCXT_SRC_SCXT_CORE_DSP_PROCESSOR_PROCESSOR_DEFS_H

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
#include "processor_defs.h"
#include "dsp/processor/processor_impl.h"

#include "sst/voice-effects/delay/Widener.h"
#include "sst/voice-effects/delay/Microgate.h"
#include "sst/voice-effects/delay/ShortDelay.h"

#include "sst/voice-effects/distortion/BitCrusher.h"
#include "sst/voice-effects/distortion/Slewer.h"
#include "sst/voice-effects/distortion/TreeMonster.h"
#include "sst/voice-effects/waveshaper/WaveShaper.h"

#include "sst/voice-effects/eq/EqNBandParametric.h"
#include "sst/voice-effects/eq/EqGraphic6Band.h"
#include "sst/voice-effects/eq/MorphEQ.h"
#include "sst/voice-effects/eq/TiltEQ.h"

#include "sst/voice-effects/filter/StaticPhaser.h"

#include "sst/voice-effects/generator/GenCorrelatedNoise.h"
#include "sst/voice-effects/generator/TiltNoise.h"
#include "sst/voice-effects/generator/EllipticBlepWaveforms.h"
#include "sst/voice-effects/generator/SinePlus.h"
#include "sst/voice-effects/generator/StringResonator.h"
#include "sst/voice-effects/generator/3opPhaseMod.h"
#include "sst/voice-effects/generator/FourVoiceResonator.h"

#include "sst/voice-effects/modulation/FreqShiftMod.h"
#include "sst/voice-effects/modulation/RingMod.h"
#include "sst/voice-effects/modulation/PhaseMod.h"
#include "sst/voice-effects/modulation/FMFilter.h"
#include "sst/voice-effects/modulation/Tremolo.h"
#include "sst/voice-effects/modulation/Phaser.h"
#include "sst/voice-effects/modulation/ShepardPhaser.h"
#include "sst/voice-effects/modulation/NoiseAM.h"
#include "sst/voice-effects/modulation/Chorus.h"
#include "sst/voice-effects/modulation/Flanger.h"
#include "sst/voice-effects/utilities/VolumeAndPan.h"
#include "sst/voice-effects/utilities/StereoTool.h"
#include "sst/voice-effects/utilities/GainMatrix.h"
#include "sst/voice-effects/dynamics/Compressor.h"
#include "sst/voice-effects/dynamics/AutoWah.h"

#include "sst/filters++.h"

#include "sst/voice-effects/filter/FiltersPlusPlus.h"
#include "sst/voice-effects/filter/UtilityFilters.h"

#include "sst/voice-effects/lifted_bus_effects/LiftedReverb1.h"
#include "sst/voice-effects/lifted_bus_effects/LiftedReverb2.h"
#include "sst/voice-effects/lifted_bus_effects/LiftedDelay.h"

namespace scxt::dsp::processor
{
// Just don't change the id or streaming name, basically

DEFINE_PROC(ShortDelay, sst::voice_effects::delay::ShortDelay<SCXTVFXConfig<1>>,
            sst::voice_effects::delay::ShortDelay<SCXTVFXConfig<2>>, proct_fx_simple_delay,
            "Simple Delay", "Delay", dsp::surgeSincTable);
PROC_DEFAULT_MIX(proct_fx_simple_delay, 0.3);

DEFINE_PROC(MicroGate, sst::voice_effects::delay::MicroGate<SCXTVFXConfig<1>>,
            sst::voice_effects::delay::MicroGate<SCXTVFXConfig<2>>, proct_fx_microgate, "MicroGate",
            "Delay", dsp::surgeSincTable);

DEFINE_PROC(BitCrusher, sst::voice_effects::distortion::BitCrusher<SCXTVFXConfig<1>>,
            sst::voice_effects::distortion::BitCrusher<SCXTVFXConfig<2>>, proct_fx_bitcrusher,
            "BitCrusher", "Distortion");
DEFINE_PROC(WaveShaper, sst::voice_effects::waveshaper::WaveShaper<SCXTVFXConfig<1>>,
            sst::voice_effects::waveshaper::WaveShaper<SCXTVFXConfig<2>>, proct_fx_waveshaper,
            "WaveShaper", "Distortion");
DEFINE_PROC(Slewer, sst::voice_effects::distortion::Slewer<SCXTVFXConfig<1>>,
            sst::voice_effects::distortion::Slewer<SCXTVFXConfig<2>>, proct_fx_slewer, "Slewer",
            "Distortion");
DEFINE_PROC(TreeMonster, sst::voice_effects::distortion::TreeMonster<SCXTVFXConfig<1>>,
            sst::voice_effects::distortion::TreeMonster<SCXTVFXConfig<2>>, proct_fx_treemonster,
            "Treemonster", "Distortion");

DEFINE_PROC(Compressor, sst::voice_effects::dynamics::Compressor<SCXTVFXConfig<1>>,
            sst::voice_effects::dynamics::Compressor<SCXTVFXConfig<2>>, proct_Compressor,
            "Compressor", "Dynamics");
DEFINE_PROC(AutoWah, sst::voice_effects::dynamics::AutoWah<SCXTVFXConfig<1>>,
            sst::voice_effects::dynamics::AutoWah<SCXTVFXConfig<2>>, proct_autowah, "Auto Wah",
            "Dynamics");

// Macros and commas don't get along
namespace procimpl::detail
{
using eq3impl = sst::voice_effects::eq::EqNBandParametric<SCXTVFXConfig<1>, 3>;
using eq3impl_os = sst::voice_effects::eq::EqNBandParametric<SCXTVFXConfig<2>, 3>;

using fastSVFImpl =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                sst::filtersplusplus::FilterModel::CytomicSVF>;
using fastSVFImpl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::CytomicSVF>;

using vemberImpl =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                sst::filtersplusplus::FilterModel::VemberClassic>;
using vemberImpl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::VemberClassic>;

using k35Impl = sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                            sst::filtersplusplus::FilterModel::K35>;
using k35Impl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::K35>;
using diodeLadderImpl =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                sst::filtersplusplus::FilterModel::DiodeLadder>;
using diodeLadderImpl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::DiodeLadder>;
using vintageImpl =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                sst::filtersplusplus::FilterModel::VintageLadder>;
using vintageImpl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::VintageLadder>;

using obx4Impl =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                sst::filtersplusplus::FilterModel::OBXD_4Pole>;
using obx4Impl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::OBXD_4Pole>;
using obxpandImpl =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                sst::filtersplusplus::FilterModel::OBXD_Xpander>;
using obxpandImpl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::OBXD_Xpander>;

using cWarpImpl =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                sst::filtersplusplus::FilterModel::CutoffWarp>;
using cWarpImpl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::CutoffWarp>;

using rWarpImpl =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                sst::filtersplusplus::FilterModel::ResonanceWarp>;
using rWarpImpl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::ResonanceWarp>;

using tripoleImpl =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                sst::filtersplusplus::FilterModel::TriPole>;
using tripoleImpl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::TriPole>;

using SnHImpl =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                sst::filtersplusplus::FilterModel::SampleAndHold>;
using SnHImpl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::SampleAndHold>;
using combImpl =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<1>,
                                                sst::filtersplusplus::FilterModel::Comb>;
using combImpl_os =
    sst::voice_effects::filter::FiltersPlusPlus<SCXTVFXConfig<2>,
                                                sst::filtersplusplus::FilterModel::Comb>;
} // namespace procimpl::detail

DEFINE_PROC(CytomicSVF, detail::fastSVFImpl, detail::fastSVFImpl_os, proct_CytomicSVF, "Fast SVF",
            "Filters");
DEFINE_PROC(VemberClassicFilter, procimpl::detail::vemberImpl, procimpl::detail::vemberImpl_os,
            proct_VemberClassic, "Vember Classic", "Filters");
DEFINE_PROC(K35Filter, procimpl::detail::k35Impl, procimpl::detail::k35Impl_os, proct_K35, "K35",
            "Filters");
DEFINE_PROC(DiodeLadderFilter, procimpl::detail::diodeLadderImpl,
            procimpl::detail::diodeLadderImpl_os, proct_diodeladder, "Linear Ladder", "Filters");
DEFINE_PROC(VintageLadder, procimpl::detail::vintageImpl, procimpl::detail::vintageImpl_os,
            proct_vintageladder, "Vintage Ladder", "Filters");
DEFINE_PROC(OBX4PFilter, procimpl::detail::obx4Impl, procimpl::detail::obx4Impl_os, proct_obx4,
            "OB-Xd 4-Pole", "Filters");
DEFINE_PROC(OBXpanderFilter, procimpl::detail::obxpandImpl, procimpl::detail::obxpandImpl_os,
            proct_obxpander, "OB-Xd Xpander", "Filters");
DEFINE_PROC(CutoffWarpFilter, procimpl::detail::cWarpImpl, procimpl::detail::cWarpImpl_os,
            proct_cutoffwarp, "Cutoff Warp", "Filters");
DEFINE_PROC(ResWarpFilter, procimpl::detail::rWarpImpl, procimpl::detail::rWarpImpl_os,
            proct_reswarp, "Resonance Warp", "Filters");
DEFINE_PROC(TripoleFilter, procimpl::detail::tripoleImpl, procimpl::detail::tripoleImpl_os,
            proct_tripole, "Tripole", "Filters");
DEFINE_PROC(SNHFilter, procimpl::detail::SnHImpl, procimpl::detail::SnHImpl_os, proct_snhfilter,
            "Sample & Hold", "Filters");

DEFINE_PROC(EQ3Band, procimpl::detail::eq3impl, procimpl::detail::eq3impl_os,
            proct_eq_3band_parametric_A, "3 Band Parametric", "EQ");
DEFINE_PROC(EQGraphic6Band, sst::voice_effects::eq::EqGraphic6Band<SCXTVFXConfig<1>>,
            sst::voice_effects::eq::EqGraphic6Band<SCXTVFXConfig<2>>, proct_eq_6band,
            "6 Band Graphic", "EQ");

DEFINE_PROC(UtilityFilters, sst::voice_effects::filter::UtilityFilters<SCXTVFXConfig<1>>,
            sst::voice_effects::filter::UtilityFilters<SCXTVFXConfig<2>>, proct_utilfilt,
            "Utility Filter", "Filters");

DEFINE_PROC(TiltEQ, sst::voice_effects::eq::TiltEQ<SCXTVFXConfig<1>>,
            sst::voice_effects::eq::TiltEQ<SCXTVFXConfig<2>>, proct_eq_tilt, "Tilt EQ", "EQ");
DEFINE_PROC(VolPan, sst::voice_effects::utilities::VolumeAndPan<SCXTVFXConfig<1>>,
            sst::voice_effects::utilities::VolumeAndPan<SCXTVFXConfig<2>>, proct_volpan,
            "Volume & Pan", "Utility");
DEFINE_PROC(StereoTool, sst::voice_effects::utilities::StereoTool<SCXTVFXConfig<1>>,
            sst::voice_effects::utilities::StereoTool<SCXTVFXConfig<2>>, proct_stereotool,
            "Stereo Tool", "Utility");
DEFINE_PROC(GainMatrix, sst::voice_effects::utilities::GainMatrix<SCXTVFXConfig<1>>,
            sst::voice_effects::utilities::GainMatrix<SCXTVFXConfig<2>>, proct_gainmatrix,
            "Gain Matrix", "Utility");
DEFINE_PROC(Widener, sst::voice_effects::delay::Widener<SCXTVFXConfig<1>>,
            sst::voice_effects::delay::Widener<SCXTVFXConfig<2>>, proct_fx_widener, "Widener",
            "Utility", dsp::surgeSincTable);

DEFINE_PROC(EllipticBlepWaveforms,
            sst::voice_effects::generator::EllipticBlepWaveforms<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::EllipticBlepWaveforms<SCXTVFXConfig<2>>,
            proct_osc_EBWaveforms, "Virtual Analog", "Generators");
PROC_DEFAULT_MIX(proct_osc_EBWaveforms, 0.5);

DEFINE_PROC(SinePlus, sst::voice_effects::generator::SinePlus<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::SinePlus<SCXTVFXConfig<2>>, proct_osc_sineplus,
            "Sine Plus", "Generators");
PROC_DEFAULT_MIX(proct_osc_sineplus, 0.5);

DEFINE_PROC(GenCorrelatedNoise, sst::voice_effects::generator::GenCorrelatedNoise<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::GenCorrelatedNoise<SCXTVFXConfig<2>>,
            proct_osc_correlatednoise, "Correlated Noise", "Generators");
PROC_DEFAULT_MIX(proct_osc_correlatednoise, 0.5);

DEFINE_PROC(GenTiltNoise, sst::voice_effects::generator::TiltNoise<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::TiltNoise<SCXTVFXConfig<2>>, proct_osc_tiltnoise,
            "Tilt Noise", "Generators");
PROC_DEFAULT_MIX(proct_osc_tiltnoise, 0.5);

DEFINE_PROC(ThreeOpPhaseMod, sst::voice_effects::generator::ThreeOpPhaseMod<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::ThreeOpPhaseMod<SCXTVFXConfig<2>>, proct_osc_3op,
            "3op Phase Mod", "Generators", dsp::twoToTheXTable, dsp::pmSineTable);

DEFINE_PROC(CombFilter, procimpl::detail::combImpl, procimpl::detail::combImpl_os, proct_comb,
            "Comb", "Resonators");

DEFINE_PROC(StringResonator, sst::voice_effects::generator::StringResonator<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::StringResonator<SCXTVFXConfig<2>>, proct_stringResonator,
            "String Resonator", "Resonators", dsp::surgeSincTable);

DEFINE_PROC(TetradResonator, sst::voice_effects::generator::FourVoiceResonator<SCXTVFXConfig<1>>,
            sst::voice_effects::generator::FourVoiceResonator<SCXTVFXConfig<2>>,
            proct_tetradResonator, "Tetrad Resonator", "Resonators", dsp::simpleSineTable);

DEFINE_PROC(MorphEQ, sst::voice_effects::eq::MorphEQ<SCXTVFXConfig<1>>,
            sst::voice_effects::eq::MorphEQ<SCXTVFXConfig<2>>, proct_eq_morph, "Morph", "Filters");
DEFINE_PROC(StaticPhaser, sst::voice_effects::filter::StaticPhaser<SCXTVFXConfig<1>>,
            sst::voice_effects::filter::StaticPhaser<SCXTVFXConfig<2>>, proct_StaticPhaser,
            "Static Phaser", "Filters");

DEFINE_PROC(FreqShiftMod, sst::voice_effects::modulation::FreqShiftMod<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::FreqShiftMod<SCXTVFXConfig<2>>, proct_fx_freqshiftmod,
            "Freqshift Mod", "Audio Rate Mod");
DEFINE_PROC(PhaseMod, sst::voice_effects::modulation::PhaseMod<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::PhaseMod<SCXTVFXConfig<2>>, proct_osc_phasemod,
            "Phase Mod", "Audio Rate Mod");
DEFINE_PROC(FMFilter, sst::voice_effects::modulation::FMFilter<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::FMFilter<SCXTVFXConfig<2>>, proct_fmfilter, "FM Filter",
            "Audio Rate Mod");
DEFINE_PROC(RingMod, sst::voice_effects::modulation::RingMod<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::RingMod<SCXTVFXConfig<2>>, proct_fx_ringmod, "Ring Mod",
            "Audio Rate Mod");
DEFINE_PROC(NoiseAM, sst::voice_effects::modulation::NoiseAM<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::NoiseAM<SCXTVFXConfig<2>>, proct_noise_am, "Noise AM",
            "Audio Rate Mod");

DEFINE_PROC(Tremolo, sst::voice_effects::modulation::Tremolo<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::Tremolo<SCXTVFXConfig<2>>, proct_Tremolo, "Tremolo",
            "Modulation");
DEFINE_PROC(Phaser, sst::voice_effects::modulation::Phaser<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::Phaser<SCXTVFXConfig<2>>, proct_Phaser, "Phaser",
            "Modulation");
DEFINE_PROC(ShepardPhaser, sst::voice_effects::modulation::ShepardPhaser<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::ShepardPhaser<SCXTVFXConfig<2>>, proct_shepard,
            "Shepard Phaser", "Modulation");
DEFINE_PROC(Chorus, sst::voice_effects::modulation::Chorus<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::Chorus<SCXTVFXConfig<2>>, proct_Chorus, "Chorus",
            "Modulation", dsp::surgeSincTable);
DEFINE_PROC(Flanger, sst::voice_effects::modulation::VoiceFlanger<SCXTVFXConfig<1>>,
            sst::voice_effects::modulation::VoiceFlanger<SCXTVFXConfig<2>>, proct_flanger,
            "Circle Flanger", "Modulation", dsp::simpleSineTable);

DEFINE_PROC(LiftedReverb1, sst::voice_effects::liftbus::LiftedReverb1<SCXTVFXConfig<1>>,
            sst::voice_effects::liftbus::LiftedReverb1<SCXTVFXConfig<2>>, proct_lifted_reverb1,
            "Reverb 1", "Reverb");
PROC_FOR_GROUP_ONLY(proct_lifted_reverb1);
PROC_DEFAULT_MIX(proct_lifted_reverb1, 0.333);

DEFINE_PROC(LiftedReverb2, sst::voice_effects::liftbus::LiftedReverb2<SCXTVFXConfig<1>>,
            sst::voice_effects::liftbus::LiftedReverb2<SCXTVFXConfig<2>>, proct_lifted_reverb2,
            "Reverb 2", "Reverb");
PROC_FOR_GROUP_ONLY(proct_lifted_reverb2);
PROC_DEFAULT_MIX(proct_lifted_reverb2, 0.333);

DEFINE_PROC(LiftedDelay, sst::voice_effects::liftbus::LiftedDelay<SCXTVFXConfig<1>>,
            sst::voice_effects::liftbus::LiftedDelay<SCXTVFXConfig<2>>, proct_lifted_delay,
            "Dual Delay", "Delay");
PROC_FOR_GROUP_ONLY(proct_lifted_delay);
PROC_DEFAULT_MIX(proct_lifted_delay, 0.333);

} // namespace scxt::dsp::processor

#endif // __SCXT_PROCESSOR_DEFS_H
