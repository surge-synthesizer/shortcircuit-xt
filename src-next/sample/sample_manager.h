
#ifndef SCXT_SRC_SAMPLE_SAMPLE_MANAGER_H
#define SCXT_SRC_SAMPLE_SAMPLE_MANAGER_H

#include "utils.h"
#include "sample.h"

#include "infrastructure/filesystem_import.h"

#include <filesystem>
#include <unordered_map>
#include <optional>
#include <vector>
#include <utility>

namespace scxt::sample
{
struct SampleManager : MoveableOnly<SampleManager>
{
    std::optional<SampleID> loadSampleByPath(const fs::path &);
    std::optional<SampleID> loadSampleByPathToID(const fs::path &, const SampleID &id);
    std::shared_ptr<Sample> getSample(const SampleID &id) const
    {
        auto p = samples.find(id);
        if (p != samples.end())
            return p->second;
        return {};
    }

    // TODO int v sample id streaming lazy
    std::vector<std::pair<SampleID, std::string>> getPathsAndIDs() const
    {
        std::vector<std::pair<SampleID, std::string>> res;
        for (const auto &[k, v] : samples)
        {
            res.emplace_back(k, v->getPath().u8string());
        }
        return res;
    }

    void reset()
    {
        samples.clear();
        streamingVersion = 0x21120101;
    }

    uint64_t streamingVersion{0x21120101}; // see comment in patch.h
    std::unordered_map<SampleID, std::shared_ptr<Sample>> samples;
};
} // namespace scxt::sample
#endif // SHORTCIRCUIT_SAMPLE_MANAGER_H
