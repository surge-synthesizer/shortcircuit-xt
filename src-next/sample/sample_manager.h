
#ifndef __SCXT_ENGINE_SAMPLE_MANAGER_H
#define __SCXT_ENGINE_SAMPLE_MANAGER_H

#include "utils.h"
#include "sample.h"

#include "infrastructure/filesystem_import.h"

#include <filesystem>
#include <unordered_map>
#include <optional>

namespace scxt::sample
{
struct SampleManager : NonCopyable<SampleManager>
{
    std::optional<SampleID> loadSampleByPath(const fs::path &);
    std::shared_ptr<Sample> getSample(const SampleID &id) const
    {
        auto p = samples.find(id);
        if (p != samples.end())
            return p->second;
        return {};
    }

    std::unordered_map<SampleID, std::shared_ptr<Sample>> samples;
};
} // namespace scxt::sample
#endif // SHORTCIRCUIT_SAMPLE_MANAGER_H
