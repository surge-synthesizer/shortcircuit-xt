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
#include "datamodel/metadata.h"
#include "sst/filters/HalfRateFilter.h"

namespace scxt::engine
{
enum BusAddress : int16_t
{
    ERROR_BUS = -2,
    DEFAULT_BUS = -1,
    MAIN_0,
    PART_0,
    AUX_0 = PART_0 + numParts
};

struct Engine;

/*
 * When you add an effect here you also need to add it to
 * - bus.h
 *      - in the toStringAvaialble
 * - bus.cpp
 *      - as an include of the effect
 *      - in createEffect
 * - src-ui/json-layout/bus-effects
 *      - add 'foo.json' as a blank ("{}") json file
 * - src-ui/componetns/MixerScreen.cpp
 *      - the menu switch
 *      - the call to 'add'
 *  - src-ui/components/mixer/PartEffectsPane.cpop
 *      - in the rebuild switch with the name of the json (Look for CS(...))
 *
 */
enum AvailableBusEffects
{
    none,
    reverb1,
    reverb2,
    flanger,
    phaser,
    delay,
    treemonster,
    nimbus,
    rotaryspeaker,
    bonsai // if you make bonsai not last, make sure to update the fromString range
};

struct BusEffectStorage
{
    BusEffectStorage() { std::fill(params.begin(), params.end(), 0.f); }
    static constexpr int maxBusEffectParams{scxt::maxBusEffectParams};
    AvailableBusEffects type{AvailableBusEffects::none};
    bool isActive{true};
    bool isTemposync{false};
    std::array<float, maxBusEffectParams> params{};
    std::array<bool, maxBusEffectParams> deact{};
    int16_t streamingVersion;

    inline bool isDeactivated(int idx) { return deact[idx]; }
    inline bool isExtended(int idx) { return false; }
};
struct BusEffect
{
    BusEffect(Engine *, BusEffectStorage *, float *) {}
    virtual ~BusEffect() = default;

    virtual void init(bool defaultsOverrideStorage) = 0;
    virtual void process(float *__restrict L, float *__restrict R) = 0;
    virtual datamodel::pmd paramAt(int i) const = 0;
    virtual int numParams() const = 0;
    virtual void onSampleRateChanged() = 0;
};

std::unique_ptr<BusEffect> createEffect(AvailableBusEffects p, Engine *e, BusEffectStorage *s);
using busRemapFn_t = void (*)(int16_t, float *const);
std::pair<int16_t, busRemapFn_t> getBusEffectRemapStreamingFunction(AvailableBusEffects);

struct Bus : MoveableOnly<Bus>, SampleRateSupport
{
    static constexpr int maxEffectsPerBus{scxt::maxEffectsPerBus};
    static constexpr int maxSendsPerBus{scxt::maxSendsPerBus};

    BusAddress address;
    Bus() : address(ERROR_BUS), downsampleFilter(6, true) {}
    Bus(BusAddress a) : address(a), downsampleFilter(6, true)
    {
        assert(address != DEFAULT_BUS && address != ERROR_BUS);
    }

    void resetBus()
    {
        busSendStorage = BusSendStorage();
        for (int i = 0; i < maxEffectsPerBus; ++i)
        {
            busEffectStorage[i] = BusEffectStorage();
            busEffects[i].reset();
        }
    }

    float output alignas(16)[2][blockSize];
    float outputOS alignas(16)[2][blockSize << 1];
    bool hasOSSignal{false}, previousHadOSSignal{false};
    float auxoutputPreFX alignas(16)[2][blockSize];
    float auxoutputPreVCA alignas(16)[2][blockSize];
    float auxoutputPostVCA alignas(16)[2][blockSize];
    float vuLevel[2]{0.f, 0.f}, vuFalloff{0.f};

    sst::filters::HalfRate::HalfRateFilter downsampleFilter;

    inline void clear()
    {
        memset(output, 0, sizeof(output));
        memset(outputOS, 0, sizeof(outputOS));
        hasOSSignal = false;
    }

    void process();

    void setAuxSendLevel(int idx, float slevel)
    {
        assert(idx >= 0 && idx < maxSendsPerBus);
        assert(busSendStorage.supportsSends);
        busSendStorage.sendLevels[idx] = slevel;
        resetSendState();
    }

    void resetSendState()
    {
        busSendStorage.hasSends = false;
        for (const auto &f : busSendStorage.sendLevels)
            busSendStorage.hasSends = busSendStorage.hasSends || (f != 0);
    }

    void setBusEffectType(Engine &e, int idx, AvailableBusEffects t);
    void initializeAfterUnstream(Engine &e);
    void sendAllBusEffectInfoToClient(const Engine &e)
    {
        for (int i = 0; i < maxEffectsPerBus; ++i)
            sendBusEffectInfoToClient(e, i);
    }
    void sendBusEffectInfoToClient(const Engine &, int slot);
    void sendBusSendStorageToClient(const Engine &e);

    void onSampleRateChanged() override;

    struct BusSendStorage
    {
        BusSendStorage()
        {
            std::fill(sendLevels.begin(), sendLevels.end(), 0.f);
            std::fill(auxLocation.begin(), auxLocation.end(), POST_VCA);
        }
        bool hasSends{false};
        bool supportsSends{false};

        enum AuxLocation
        {
            PRE_FX,
            POST_FX_PRE_VCA,
            POST_VCA
        };
        std::array<AuxLocation, maxSendsPerBus> auxLocation;
        DECLARE_ENUM_STRING(AuxLocation);

        // Send levels in 0...1 scale to each bus
        std::array<float, maxSendsPerBus> sendLevels{};

        // VCA level in raw amplitude also. UI will dbIfy this I'm sure
        bool mute{false};
        bool solo{false};
        float pan{0.f};
        float level{1.f};

        uint16_t pluginOutputBus{0};
    } busSendStorage;

    bool mutedDueToSoloAway{false};

    std::array<BusEffectStorage, maxEffectsPerBus> busEffectStorage;

    std::array<std::unique_ptr<BusEffect>, maxEffectsPerBus> busEffects;
};

inline std::string toStringAvailableBusEffects(const AvailableBusEffects &p)
{
    switch (p)
    {
    case none:
        return "none";
    case reverb1:
        return "reverb1";
    case reverb2:
        return "reverb2";
    case treemonster:
        return "treemonster";
    case flanger:
        return "flanger";
    case phaser:
        return "phaser";
    case delay:
        return "delay";
    case nimbus:
        return "nimbus";
    case rotaryspeaker:
        return "rotaryspeaker";
    case bonsai:
        return "bonsai";
    }
    return "normal";
}

inline AvailableBusEffects fromStringAvailableBusEffects(const std::string &s)
{
    static auto inverse = makeEnumInverse<AvailableBusEffects, toStringAvailableBusEffects>(
        AvailableBusEffects::none, AvailableBusEffects::bonsai);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return none;
    return p->second;
}
} // namespace scxt::engine

#endif // SHORTCIRCUITXT_BUS_H
