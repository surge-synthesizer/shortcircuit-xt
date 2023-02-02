#ifndef __SCXT_ENGINE_ZONE_H
#define __SCXT_ENGINE_ZONE_H

#include <array>

#include "utils.h"
#include "keyboard.h"
#include "sample/sample_manager.h"
#include "dsp/filter/filter.h"
#include "configuration.h"
#include "modulation/voice_matrix.h"
#include "modulation/modulators/steplfo.h"

namespace scxt::voice
{
struct Voice;
}

namespace scxt::engine
{
struct Group;

constexpr int filtersPerZone{2};
constexpr int lfosPerZone{3};

struct Zone : NonCopyable<Zone>
{
    Zone() : id(ZoneID::next()) { initialize(); }
    Zone(SampleID sid) : id(ZoneID::next()), sampleID(sid) { initialize(); }

    ZoneID id;
    SampleID sampleID;
    std::shared_ptr<sample::Sample> sample{nullptr};

    float output alignas(16)[maxOutputs][2][blockSize];
    void process();

    // TODO: Multi-output
    size_t getNumOutputs() const { return 1; }

    bool attachToSample(const sample::SampleManager &manager)
    {
        if (sampleID.isValid())
        {
            sample = manager.getSample(sampleID);
        }
        else
        {
            sample.reset();
        }
        return sample != nullptr;
    }

    // TODO: This becomes 'meta' eventually probably
    KeyboardRange keyboardRange;
    uint8_t rootKey{60};

    // TODO - this should probably be a filterstorage object
    dsp::filter::FilterType filterType[filtersPerZone]{dsp::filter::ft_none, dsp::filter::ft_none};
    float filterMix[filtersPerZone]{0, 0};

    Group *parentGroup{nullptr};

    bool isActive() { return activeVoices != 0; }
    uint32_t activeVoices{0};
    std::array<voice::Voice *, maxVoices> voiceWeakPointers;
    void initialize()
    {
        for (auto &v : voiceWeakPointers)
            v = nullptr;

        for (auto &l : lfoStorage)
        {
            modulation::modulators::load_lfo_preset(modulation::modulators::lp_clear, &l);
        }
    }
    // Just a weak ref - don't take ownership. engine manages lifetime
    void addVoice(voice::Voice *);
    void removeVoice(voice::Voice *);

    std::array<modulation::VoiceModMatrix::Routing, modulation::numVoiceRoutingSlots> routingTable;
    modulation::modulators::StepLFOStorage lfoStorage[lfosPerZone];
};
} // namespace scxt::engine

#endif