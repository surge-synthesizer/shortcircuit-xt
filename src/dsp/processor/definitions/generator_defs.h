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

#ifndef SCXT_SRC_DSP_PROCESSOR_DEFINITIONS_GENERATOR_DEFS_H
#define SCXT_SRC_DSP_PROCESSOR_DEFINITIONS_GENERATOR_DEFS_H

#include "dsp/processor/processor_impl.h"
#include "sst/voice-effects/generator/GenCorrelatedNoise.h"
#include "sst/voice-effects/generator/GenSin.h"
#include "sst/voice-effects/generator/GenSaw.h"
#include "sst/voice-effects/generator/GenPulseSync.h"
#include "sst/voice-effects/generator/GenPhaseMod.h"

namespace scxt::dsp::processor
{
namespace generator
{

struct alignas(16) GenSin : SSTVoiceEffectShim<sst::voice_effects::generator::GenSin<SCXTVFXConfig>>
{
    static constexpr const char *processorName{"Sin"};
    static constexpr const char *processorStreamingName{"osc-sin"};
    static constexpr const char *processorDisplayGroup{"Generators"};

    GenSin(engine::MemoryPool *mp, float *f, int32_t *i)
    {
        setupProcessor(this, proct_osc_sin, mp, f, i);
    }
};

struct alignas(16) GenSaw : SSTVoiceEffectShim<sst::voice_effects::generator::GenSaw<SCXTVFXConfig>>
{
    static constexpr const char *processorName{"Saw"};
    static constexpr const char *processorStreamingName{"osc-saw"};
    static constexpr const char *processorDisplayGroup{"Generators"};

    GenSaw(engine::MemoryPool *mp, float *f, int32_t *i)
    {
        setupProcessor(this, proct_osc_saw, mp, f, i);
    }
};

struct alignas(16) GenPulseSync
    : SSTVoiceEffectShim<sst::voice_effects::generator::GenPulseSync<SCXTVFXConfig>>
{
    static constexpr const char *processorName{"Pulse Sync"};
    static constexpr const char *processorStreamingName{"osc--pulse-sync"};
    static constexpr const char *processorDisplayGroup{"Generators"};

    GenPulseSync(engine::MemoryPool *mp, float *f, int32_t *i)
        : SSTVoiceEffectShim<sst::voice_effects::generator::GenPulseSync<SCXTVFXConfig>>(
              dsp::sincTable)
    {
        setupProcessor(this, proct_osc_pulse_sync, mp, f, i);
    }
};

struct alignas(16) GenPhaseMod
    : SSTVoiceEffectShim<sst::voice_effects::generator::GenPhaseMod<SCXTVFXConfig>>
{
    static constexpr const char *processorName{"Phase Mod"};
    static constexpr const char *processorStreamingName{"osc-phase-mod"};
    static constexpr const char *processorDisplayGroup{"Generators"};

    GenPhaseMod(engine::MemoryPool *mp, float *f, int32_t *i)
    {
        setupProcessor(this, proct_osc_phasemod, mp, f, i);
    }
};

struct alignas(16) GenCorrelatedNoise
    : SSTVoiceEffectShim<sst::voice_effects::generator::GenCorrelatedNoise<SCXTVFXConfig>>
{
    static constexpr const char *processorName{"Noise"};
    static constexpr const char *processorStreamingName{"osc-correlated-noise"};
    static constexpr const char *processorDisplayGroup{"Generators"};

    GenCorrelatedNoise(engine::MemoryPool *mp, float *f, int32_t *i)
    {
        setupProcessor(this, proct_osc_correlatednoise, mp, f, i);
    }
};
} // namespace generator

template <> struct ProcessorImplementor<ProcessorType::proct_osc_sin>
{
    typedef generator::GenSin T;
    static_assert(sizeof(T) < processorMemoryBufferSize);
};

template <> struct ProcessorImplementor<ProcessorType::proct_osc_saw>
{
    typedef generator::GenSaw T;
    static_assert(sizeof(T) < processorMemoryBufferSize);
};

template <> struct ProcessorImplementor<ProcessorType::proct_osc_pulse_sync>
{
    typedef generator::GenPulseSync T;
    static_assert(sizeof(T) < processorMemoryBufferSize);
};

template <> struct ProcessorImplementor<ProcessorType::proct_osc_phasemod>
{
    typedef generator::GenPhaseMod T;
    static_assert(sizeof(T) < processorMemoryBufferSize);
};

template <> struct ProcessorImplementor<ProcessorType::proct_osc_correlatednoise>
{
    typedef generator::GenCorrelatedNoise T;
    static_assert(sizeof(T) < processorMemoryBufferSize);
};

} // namespace scxt::dsp::processor

#endif // SHORTCIRCUITXT_GENERATOR_DEFS_H
