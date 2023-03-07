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
    const ThreadingChecker &threadingChecker;
    SampleManager(const ThreadingChecker &t) : threadingChecker(t) {}

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
