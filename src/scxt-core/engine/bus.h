/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#ifndef SCXT_SRC_SCXT_CORE_ENGINE_BUS_H
#define SCXT_SRC_SCXT_CORE_ENGINE_BUS_H

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

#include "bus_effect.h"

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
std::string getBusAddressLabel(BusAddress b, const std::string &defaultName = "Default",
                               bool shortName = false);

struct Engine;

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
    bool inRingout{false}, wasInRingout{false};
    void setInRingout(bool b) { inRingout = b; }

    sst::filters::HalfRate::HalfRateFilter downsampleFilter;

    inline void clear()
    {
        memset(output, 0, sizeof(output));
        memset(outputOS, 0, sizeof(outputOS));
        hasOSSignal = false;
        inRingout = true;
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
} // namespace scxt::engine

#endif // SHORTCIRCUITXT_BUS_H
