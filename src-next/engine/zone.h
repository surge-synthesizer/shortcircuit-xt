#ifndef __SCXT_ENGINE_ZONE_H
#define __SCXT_ENGINE_ZONE_H

#include "utils.h"
#include "keyboard.h"
#include "sample/sample_manager.h"
#include "dsp/filter/filter.h"

namespace scxt::engine
{
struct Group;

constexpr int filtersPerZone{2};
struct Zone : NonCopyable<Zone>
{
    Zone() : id(ZoneID::next()) {}
    Zone(SampleID sid) : id(ZoneID::next()), sampleID(sid) {}

    ZoneID id;
    SampleID sampleID;
    std::shared_ptr<sample::Sample> sample{nullptr};

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
};
} // namespace scxt::engine

#endif