//
// Created by Paul Walker on 1/30/23.
//

#ifndef __SCXT_VOICE_VOICE_H
#define __SCXT_VOICE_VOICE_H

#include "engine/zone.h"
#include "dsp/sinc_table.h"
#include "dsp/generator.h"
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

    Voice(const engine::Zone &z) : zone(z)
    {
        memset(output, 0, 2 * blockSize * sizeof(float));
        dsp::sincTable.init();
    }

    /**
     * Generate the next blocksize of data into output
     * @return false if you cant
     */
    bool process();

    /**
     * Initialize the dsp generator state
     */
    void initializeGenerator();

    /**
     * Calculates the playback speed. This is also where we put tuning
     */
    void calculateGeneratorRatio();

    double sampleRate{-1}, sampleRateInv{-1};

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
    }
    void release() { playState = OFF; }
};
} // namespace scxt::voice

#endif // __SCXT_VOICE_VOICE_H
