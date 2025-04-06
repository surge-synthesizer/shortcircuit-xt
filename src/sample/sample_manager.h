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
#include "gig.h"
#include <miniz.h>

namespace scxt::sample
{
struct ZipArchiveHolder : scxt::MoveableOnly<ZipArchiveHolder>
{
    mz_zip_archive zip_archive;
    bool isOpen{false};
    ZipArchiveHolder(const fs::path &p)
    {
        memset(&zip_archive, 0, sizeof(zip_archive));
        isOpen = mz_zip_reader_init_file(&zip_archive, p.u8string().c_str(), 0);
    }
    ~ZipArchiveHolder()
    {
        if (isOpen)
        {
            mz_zip_reader_end(&zip_archive);
        }
    }

    void close()
    {
        if (isOpen)
        {
            mz_zip_reader_end(&zip_archive);
            isOpen = false;
        }
    }
};

struct SampleManager : MoveableOnly<SampleManager>
{
    const ThreadingChecker &threadingChecker;
    SampleManager(const ThreadingChecker &t) : threadingChecker(t) {}
    ~SampleManager();

    void addSampleAsMissing(const SampleID &id, const Sample::SampleFileAddress &f);
    std::optional<SampleID> loadSampleByFileAddress(const Sample::SampleFileAddress &,
                                                    const SampleID &);

    std::optional<SampleID> loadSampleByPath(const fs::path &);

    std::optional<SampleID> loadSampleFromSF2(const fs::path &,
                                              sf2::File *f, // if this is null I will re-open it
                                              int preset, int instrument, int region);
    int findSF2SampleIndexFor(sf2::File *, int preset, int instrument, int region);

    std::optional<SampleID> loadSampleFromGIG(const fs::path &,
                                              gig::File *f, // if this is null I will re-open it
                                              int preset, int instrument, int region);

    std::optional<SampleID>
    loadSampleFromSCXTMonolith(const fs::path &,
                               RIFF::File *f, // if this is null I will re-open it
                               int preset, int instrument, int region);

    std::optional<SampleID> setupSampleFromMultifile(const fs::path &, const std::string &md5,
                                                     int idx, void *data, size_t dataSize);
    std::optional<SampleID> loadSampleFromMultiSample(const fs::path &, int idx,
                                                      const SampleID &id);

    std::shared_ptr<Sample> getSample(const SampleID &id) const
    {
        auto p = samples.find(id);
        if (p != samples.end())
            return p->second;

        auto alias = idAliases.find(id);
        if (alias != idAliases.end())
        {
            return getSample(alias->second);
        }
        return {};
    }

    fs::path reparentPath;
    static constexpr const char *relativeSentinel = "SCXT_RELATIVE_PATH_MARKER";
    void reparentSamplesOnStreamToRelative(const fs::path &newParent)
    {
        reparentPath = fs::path{relativeSentinel} / newParent;
    }
    void clearReparenting() { reparentPath = fs::path{}; }

    fs::path relativeRoot;
    void setRelativeRoot(const fs::path &p) { relativeRoot = p; }
    void clearRelativeRoot() { relativeRoot = fs::path{}; }

    std::map<fs::path, int> monolithIndex;
    fs::path monolithPath;

    void setMonolithBinaryIndex(const fs::path &p, const std::vector<fs::path> &idx)
    {
        monolithPath = p;
        monolithIndex.clear();
        int fi{0};
        for (auto &p : idx)
        {
            monolithIndex[p] = fi;
            fi++;
        }
    }
    void clearMonolithBinaryIndex()
    {
        monolithPath = fs::path{};
        monolithIndex.clear();
    }

    typedef std::vector<std::pair<SampleID, Sample::SampleFileAddress>> sampleAddressesAndIds_t;
    sampleAddressesAndIds_t getSampleAddressesAndIDs() const
    {
        sampleAddressesAndIds_t res;
        for (const auto &[k, v] : samples)
        {
            auto sfa = v->getSampleFileAddress();
            if (!reparentPath.empty())
            {
                auto prior = sfa.path;
                sfa.path = reparentPath / sfa.path.filename();
                SCLOG_IF(sampleLoadAndPurge, "SampleManager::getSampleAddressesAndIDs: reparenting "
                                                 << prior << " to " << sfa.path);
            }
            res.emplace_back(k, sfa);
        }
        return res;
    }

    sampleAddressesAndIds_t getSampleAddressesFor(const std::vector<SampleID> &) const;

    void restoreFromSampleAddressesAndIDs(const sampleAddressesAndIds_t &);

    void purgeUnreferencedSamples();

    void reset()
    {
        samples.clear();
        sf2FilesByPath.clear();
        streamingVersion = 0x2112'01'01;
        updateSampleMemory();
    }

    uint64_t streamingVersion{0x2112'01'01}; // see comment in patch.h

    std::atomic<uint64_t> sampleMemoryInBytes{0};

    void addIdAlias(const SampleID &from, const SampleID &to) { idAliases[from] = to; }

    SampleID resolveAlias(const SampleID &a)
    {
        auto ap = idAliases.find(a);
        if (ap == idAliases.end())
        {
            return a;
        }
        assert(ap->second != a);
        return resolveAlias(ap->second);
    }

    using sampleMap_t = std::unordered_map<SampleID, std::shared_ptr<Sample>>;

    // You can do a const interation but not a non-const one
    sampleMap_t::const_iterator samplesBegin() const { return samples.cbegin(); }
    sampleMap_t::const_iterator samplesEnd() const { return samples.cend(); }

  private:
    void updateSampleMemory();
    std::unordered_map<SampleID, SampleID> idAliases;

    sampleMap_t samples;

    std::unordered_map<std::string, std::tuple<std::unique_ptr<RIFF::File>,
                                               std::unique_ptr<sf2::File>>>
        sf2FilesByPath; // last is the md5sum
    std::unordered_map<std::string, std::string> sf2MD5ByPath;
    std::unordered_map<std::string, std::tuple<std::unique_ptr<RIFF::File>,
                                               std::unique_ptr<gig::File>>>
        gigFilesByPath; // last is the md5sum
    std::unordered_map<std::string, std::string> gigMD5ByPath;

    std::unordered_map<std::string, std::unique_ptr<RIFF::File>>
        scxtMonolithFilesByPath; // last is the md5sum
    std::unordered_map<std::string, std::string> scxtMonolithMD5ByPath;

    std::unordered_map<std::string, std::unique_ptr<ZipArchiveHolder>> zipArchives;
};
} // namespace scxt::sample
#endif // SHORTCIRCUIT_SAMPLE_MANAGER_H
