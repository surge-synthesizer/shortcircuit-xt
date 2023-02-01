#ifndef __SCXT_ENGINE_ZONE_H
#define __SCXT_ENGINE_ZONE_H

#include <array>

#include "utils.h"
#include "keyboard.h"
#include "sample/sample_manager.h"
#include "dsp/filter/filter.h"
#include "configuration.h"

namespace scxt::voice
{
struct Voice;
}

namespace scxt::engine
{
struct Group;

constexpr int filtersPerZone{2};
struct Zone : NonCopyable<Zone>
{
    Zone() : id(ZoneID::next()) { clearVoices(); }
    Zone(SampleID sid) : id(ZoneID::next()), sampleID(sid) { clearVoices(); }

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

    dsp::filter::FilterType filterType[filtersPerZone]{dsp::filter::ft_none, dsp::filter::ft_none};

    Group *parentGroup{nullptr};


    bool isActive() { return activeVoices != 0; }
    uint32_t activeVoices{0};
    std::array<voice::Voice *, maxVoices> voiceWeakPointers;
    void clearVoices()
    {
        for (auto &v : voiceWeakPointers)
            v = nullptr;
    }
    // Just a weak ref - don't take ownership. engine manages lifetime
    void addVoice(voice::Voice *);
    void removeVoice(voice::Voice *);
};
} // namespace scxt::engine

#endif