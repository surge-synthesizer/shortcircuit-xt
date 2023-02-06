//
// Created by Paul Walker on 1/30/23.
//

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
    dsp::processor::ProcessorType processorType[engine::processorsPerZone]{dsp::processor::proct_none,
                                                                           dsp::processor::proct_none};
    uint8_t processorPlacementStorage alignas(
        16)[engine::processorsPerZone][dsp::processor::processorMemoryBufferSize];
    float processorFloatParams alignas(
        16)[engine::processorsPerZone][dsp::processor::maxProcessorFloatParams];
    int32_t processorIntParams alignas(
        16)[engine::processorsPerZone][dsp::processor::maxProcessorIntParams];

    void initializeProcessors();

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
        std::cout << "   Voice Attack " << zone->sample->getPath().u8string() << std::endl;
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
