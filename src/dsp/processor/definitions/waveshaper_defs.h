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

#ifndef SCXT_SRC_DSP_PROCESSOR_DEFINITIONS_WAVESHAPER_DEFS_H
#define SCXT_SRC_DSP_PROCESSOR_DEFINITIONS_WAVESHAPER_DEFS_H

#include "dsp/processor/processor_impl.h"
#include "sst/voice-effects/waveshaper/WaveShaper.h"

namespace scxt::dsp::processor
{
namespace waveshaper
{

struct alignas(16) WaveShaper
    : SSTVoiceEffectShim<sst::voice_effects::waveshaper::WaveShaper<SCXTVFXConfig>>
{
    static constexpr const char *processorName{"WaveShaper"};
    static constexpr const char *processorStreamingName{"waveshaper-fx"};
    static constexpr const char *processorDisplayGroup{"Distortion"};

    WaveShaper(engine::MemoryPool *mp, float *f, int32_t *i)
    {
        setupProcessor(this, proct_fx_waveshaper, mp, f, i);
    }
    virtual ~WaveShaper() = default;
};
} // namespace waveshaper

template <> struct ProcessorImplementor<ProcessorType::proct_fx_waveshaper>
{
    typedef waveshaper::WaveShaper T;
    static_assert(sizeof(T) < processorMemoryBufferSize);
};

} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_WAVESHAPER_DEFS_H
