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
#include "SF.h"

namespace scxt::sample
{
struct SampleManager : MoveableOnly<SampleManager>
{
    const ThreadingChecker &threadingChecker;
    SampleManager(const ThreadingChecker &t) : threadingChecker(t) {}
    ~SampleManager();

    std::optional<SampleID> loadSampleByFileAddress(const Sample::SampleFileAddress &);
    std::optional<SampleID> loadSampleByFileAddressToID(const Sample::SampleFileAddress &,
                                                        const SampleID &);

    std::optional<SampleID> loadSampleByPath(const fs::path &);
    std::optional<SampleID> loadSampleByPathToID(const fs::path &, const SampleID &id);

    std::optional<SampleID> loadSampleFromSF2(const fs::path &,
                                              sf2::File *f, // if this is null I will re-open it
                                              int instrument, int region);
    std::optional<SampleID> loadSampleFromSF2ToID(const fs::path &,
                                                  sf2::File *f, // if this is null I will re-open it
                                                  int instrument, int region, const SampleID &id);

    std::shared_ptr<Sample> getSample(const SampleID &id) const
    {
        auto p = samples.find(id);
        if (p != samples.end())
            return p->second;
        return {};
    }

    typedef std::vector<std::pair<SampleID, Sample::SampleFileAddress>> sampleAddressesAndIds_t;
    sampleAddressesAndIds_t getSampleAddressesAndIDs() const
    {
        sampleAddressesAndIds_t res;
        for (const auto &[k, v] : samples)
        {
            res.emplace_back(k, v->getSampleFileAddress());
        }
        return res;
    }
    void restoreFromSampleAddressesAndIDs(const sampleAddressesAndIds_t &);

    void purgeUnreferencedSamples();

    void reset()
    {
        samples.clear();
        sf2FilesByPath.clear();
        streamingVersion = 0x21120101;
    }

    std::vector<fs::path> missingList;
    void resetMissingList() { missingList.clear(); }

    uint64_t streamingVersion{0x21120101}; // see comment in patch.h
    std::unordered_map<SampleID, std::shared_ptr<Sample>> samples;

  private:
    std::unordered_map<std::string,
                       std::pair<std::unique_ptr<RIFF::File>, std::unique_ptr<sf2::File>>>
        sf2FilesByPath;
};
} // namespace scxt::sample
#endif // SHORTCIRCUIT_SAMPLE_MANAGER_H
