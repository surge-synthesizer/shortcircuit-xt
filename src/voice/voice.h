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

#ifndef SCXT_SRC_VOICE_VOICE_H
#define SCXT_SRC_VOICE_VOICE_H

#include "engine/zone.h"
#include "dsp/sinc_table.h"
#include "dsp/generator.h"
#include "dsp/processor/processor.h"

#include "modulation/voice_matrix.h"

#include "vembertech/vt_dsp/lipol.h"
#include "modulation/modulators/steplfo.h"

#include "configuration.h"

#include "sst/basic-blocks/modulators/ADSREnvelope.h"

namespace scxt::voice
{
struct alignas(16) Voice : MoveableOnly<Voice>, SampleRateSupport
{
    float output alignas(16)[2][blockSize];
    engine::Zone *zone{nullptr}; // I do *not* own this. The engine guarantees it outlives the voice

    dsp::GeneratorState GD;
    dsp::GeneratorIO GDIO;
    dsp::GeneratorFPtr Generator;

    int16_t channel{0};
    uint8_t key{60};
    int32_t noteId{-1};

    modulation::VoiceModMatrix modMatrix;
    modulation::modulators::StepLFO lfos[engine::lfosPerZone];
    typedef sst::basic_blocks::modulators::ADSREnvelope<
        Voice, blockSize, sst::basic_blocks::modulators::ThirtyTwoSecondRange>
        adsrenv_t;
    adsrenv_t aeg, eg2;
    // TODO obviously this sucks move to a table
    inline float envelope_rate_linear_nowrap(float f)
    {
        return blockSize * sampleRateInv * pow(2.f, -f);
    }

    template <typename ET, int EB, typename ER>
    friend struct sst::basic_blocks::modulators::ADSREnvelope;

    Voice(engine::Zone *z);

    ~Voice();

    /**
     * Generate the next blocksize of data into output
     * @return false if you cant
     */
    bool process();

    /**
     * Voice Setup
     */
    void voiceStarted();

    /**
     * Initialize the dsp generator state
     */
    void initializeGenerator();

    /**
     * Calculates the playback speed. This is also where we put tuning
     */
    void calculateGeneratorRatio();

    /**
     * Processors: Storage, memory blocks, types, and more
     */
    dsp::processor::Processor *processors[engine::processorsPerZone]{nullptr, nullptr};
    dsp::processor::ProcessorType processorType[engine::processorsPerZone]{
        dsp::processor::proct_none, dsp::processor::proct_none};
    uint8_t processorPlacementStorage alignas(
        16)[engine::processorsPerZone][dsp::processor::processorMemoryBufferSize];
    int32_t processorIntParams alignas(
        16)[engine::processorsPerZone][dsp::processor::maxProcessorIntParams];

    void initializeProcessors();

    // TODO - this should be more carefully structured for modulation onto the entire filter
    lipol_ps processorMix[engine::processorsPerZone];

    // TODO This is obvious garbage from hereon down
    size_t sp{0};
    enum PlayState
    {
        GATED,
        RELEASED,
        FINISHED,
        CLEANUP,
        OFF
    } playState{OFF};

    void attack()
    {
        assert(zone->sample);
        playState = GATED;
        voiceStarted();
    }
    void release()
    {
        if (playState == FINISHED)
        {
            playState = CLEANUP;
        }
        else
        {
            playState = RELEASED;
        }
    }

    void cleanupVoice()
    {
        zone->removeVoice(this);
        playState = OFF;
    }
};
} // namespace scxt::voice

#endif // __SCXT_VOICE_VOICE_H
