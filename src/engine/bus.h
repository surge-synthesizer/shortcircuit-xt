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

#ifndef SCXT_SRC_ENGINE_BUS_H
#define SCXT_SRC_ENGINE_BUS_H

/*
 * A bus is a single stereo object which accumulates samples,
 * either directly or additively, runs effects, and has a VCA.
 * It may accumulate more
 *
 * Basically its used to add the auxes and mains in the engine,
 * and also each part has a bus. So part has one bus;
 * engine has 4 aux + one main.
 *
 * Do not confuse bus with physical output. SC VST3 has one or
 * 20 outputs, and the engine routes busses to them dependeing on
 * configuration. In order to make this routing easy, though
 * a bus has an output index which the engine can consume
 * and also has (in some cases) send levels to indices.
 */

#include <array>
#include <memory>
#include <cassert>

#include "configuration.h"
#include "utils.h"

namespace scxt::engine
{
enum BusAddress : int16_t
{
    DEFAULT_BUS = -1,
    MAIN_0,
    AUX_0 = MAIN_0 + numParts
};

struct Engine;

struct BusEffectStorage
{
    static constexpr int maxBusEffectParams{12};
    float params[maxBusEffectParams];
};
struct BusEffect
{
    BusEffect(Engine *, BusEffectStorage *, float *) {}
    virtual ~BusEffect() = default;

    virtual void init() = 0;
    virtual void process(float *__restrict L, float *__restrict R) = 0;
};

enum AvailableBusEffects
{
    reverb1,
    flanger
};

std::unique_ptr<BusEffect> createEffect(AvailableBusEffects p, Engine *e, BusEffectStorage *s);

struct Bus : MoveableOnly<Bus>, SampleRateSupport
{
    static constexpr int maxEffectsPerBus{4};
    static constexpr int maxSendsPerBus{4};

    float output alignas(16)[2][blockSize];
    float auxoutput alignas(16)[2][blockSize];

    inline void clear() { memset(output, 0, sizeof(output)); }

    void process();

    bool hasSends{false};
    bool supportsSends{false};
    bool supportsEffects{false};

    enum AuxLocation
    {
        PRE_FX,
        POST_FX_PRE_VCA,
        POST_VCA
    } auxLocation{POST_FX_PRE_VCA};

    void setAuxSendLevel(int idx, float slevel)
    {
        assert(idx >= 0 && idx < maxSendsPerBus);
        assert(supportsSends);
        sendLevels[idx] = slevel;
        hasSends = false;
        for (const auto &f : sendLevels)
            hasSends = hasSends || (f != 0);
    }

  private:
    // Send levels in 0...1 scale to each bus
    std::array<float, maxSendsPerBus> sendLevels{};
    // VCA level in raw amplitude also. UI will dbIfy this I'm sure
    float level{1.f};

    std::array<std::unique_ptr<BusEffect>, maxEffectsPerBus> busEffects;
    std::array<BusEffectStorage, maxEffectsPerBus> busEffectStorage;
};

} // namespace scxt::engine

#endif // SHORTCIRCUITXT_BUS_H
