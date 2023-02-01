//
// Created by Paul Walker on 1/30/23.
//

#ifndef __SCXT_VOICE_VOICE_H
#define __SCXT_VOICE_VOICE_H

#include "engine/zone.h"
#include "dsp/sinc_table.h"
#include "dsp/generator.h"

#include "dsp/filter/filter.h"

#include "vembertech/vt_dsp/lipol.h"

#include "configuration.h"

namespace scxt::voice
{
struct alignas(16) Voice : NonCopyable<Voice>
{
    float output alignas(16)[2][blockSize];
    const engine::Zone &zone;

    dsp::GeneratorState GD;
    dsp::GeneratorIO GDIO;
    dsp::GeneratorFPtr Generator;

    uint8_t key{60};

    Voice(const engine::Zone &z);

    ~Voice();

    /**
     * Generate the next blocksize of data into output
     * @return false if you cant
     */
    bool process();

    /**
     * Snap values on construction from the zone
     */
    void initializeFromZone();

    /**
     * Initialize the dsp generator state
     */
    void initializeGenerator();

    /**
     * Calculates the playback speed. This is also where we put tuning
     */
    void calculateGeneratorRatio();

    double sampleRate{-1}, sampleRateInv{-1};

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
        OFF
    } playState{OFF};
    void attack()
    {
        assert(zone.sample);
        std::cout << "   Voice Attack " << zone.sample->getPath().u8string() << std::endl;
        playState = GATED;
        initializeGenerator();
        initializeFilters();
    }
    void release() { playState = OFF; }
};
} // namespace scxt::voice

#endif // __SCXT_VOICE_VOICE_H
