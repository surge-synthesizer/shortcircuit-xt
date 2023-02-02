//
// Created by Paul Walker on 1/30/23.
//

#ifndef __SCXT_VOICE_VOICE_H
#define __SCXT_VOICE_VOICE_H

#include "engine/zone.h"
#include "dsp/sinc_table.h"
#include "dsp/generator.h"
#include "dsp/filter/filter.h"

#include "modulation/voice_matrix.h"

#include "vembertech/vt_dsp/lipol.h"
#include "modulation/modulators/steplfo.h"

#include "configuration.h"

namespace scxt::voice
{
struct alignas(16) Voice : NonCopyable<Voice>, SampleRateSupport
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
     * Filters: Storage, memory blocks, types, and more
     */
    dsp::filter::Filter *filters[engine::filtersPerZone]{nullptr, nullptr};
    dsp::filter::FilterType filterType[engine::filtersPerZone]{dsp::filter::ft_none,
                                                               dsp::filter::ft_none};
    uint8_t filterStorage alignas(16)[engine::filtersPerZone][dsp::filter::filterMemoryBufferSize];
    float filterFloatParams alignas(16)[engine::filtersPerZone][dsp::filter::maxFilterFloatParams];
    int32_t filterIntParams alignas(16)[engine::filtersPerZone][dsp::filter::maxFilterIntParams];

    void initializeFilters();

    lipol_ps fmix[engine::filtersPerZone];

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
