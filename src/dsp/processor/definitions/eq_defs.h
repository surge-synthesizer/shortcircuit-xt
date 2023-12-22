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

#ifndef SCXT_SRC_DSP_PROCESSOR_DEFINITIONS_EQ_DEFS_H
#define SCXT_SRC_DSP_PROCESSOR_DEFINITIONS_EQ_DEFS_H

#include "dsp/processor/processor_impl.h"
#include "sst/voice-effects/eq/EqNBandParametric.h"

namespace scxt::dsp::processor
{
namespace eq
{

struct alignas(16) EQ1Band
    : SSTVoiceEffectShim<sst::voice_effects::eq::EqNBandParametric<SCXTVFXConfig, 1>>
{
    static constexpr const char *processorName{"1 Band Parametric"};
    static constexpr const char *processorStreamingName{"eq-parm-1band"};
    static constexpr const char *processorDisplayGroup{"EQ"};

    EQ1Band(engine::MemoryPool *mp, float *f, int32_t *i)
    {
        setupProcessor(this, proct_eq_1band_parametric_A, mp, f, i);
    }
};

struct alignas(16) EQ2Band
    : SSTVoiceEffectShim<sst::voice_effects::eq::EqNBandParametric<SCXTVFXConfig, 2>>
{
    static constexpr const char *processorName{"2 Band Parametric"};
    static constexpr const char *processorStreamingName{"eq-parm-2band"};
    static constexpr const char *processorDisplayGroup{"EQ"};

    EQ2Band(engine::MemoryPool *mp, float *f, int32_t *i)
    {
        setupProcessor(this, proct_eq_2band_parametric_A, mp, f, i);
    }
};

struct alignas(16) EQ3Band
    : SSTVoiceEffectShim<sst::voice_effects::eq::EqNBandParametric<SCXTVFXConfig, 3>>
{
    static constexpr const char *processorName{"3 Band Parametric"};
    static constexpr const char *processorStreamingName{"eq-parm-3band"};
    static constexpr const char *processorDisplayGroup{"EQ"};

    EQ3Band(engine::MemoryPool *mp, float *f, int32_t *i)
    {
        setupProcessor(this, proct_eq_3band_parametric_A, mp, f, i);
    }
};

} // namespace eq

template <> struct ProcessorImplementor<ProcessorType::proct_eq_1band_parametric_A>
{
    typedef eq::EQ1Band T;
    static_assert(sizeof(T) < processorMemoryBufferSize);
};

template <> struct ProcessorImplementor<ProcessorType::proct_eq_2band_parametric_A>
{
    typedef eq::EQ2Band T;
    static_assert(sizeof(T) < processorMemoryBufferSize);
};

template <> struct ProcessorImplementor<ProcessorType::proct_eq_3band_parametric_A>
{
    typedef eq::EQ3Band T;
    static_assert(sizeof(T) < processorMemoryBufferSize);
};

} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_WAVESHAPER_DEFS_H
