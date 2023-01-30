#ifndef __SCXT_ENGINE_ZONE_H
#define __SCXT_ENGINE_ZONE_H

#include "utils.h"
#include "keyboard.h"
#include "sample/sample_manager.h"

namespace scxt::engine
{
struct Group;

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

    KeyboardRange keyboardRange;
    uint8_t rootKey{60};

    Group *parentGroup{nullptr};
};
} // namespace scxt::engine

#endif