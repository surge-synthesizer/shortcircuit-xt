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

#include <cassert>
#include "sample_manager.h"

namespace scxt::sample
{

void SampleManager::restoreFromSampleAddressesAndIDs(const sampleAddressesAndIds_t &r)
{
    for (const auto &[id, addr] : r)
    {
        if (!fs::exists(addr.path))
        {
            missingList.push_back(addr.path);
        }
        else
        {
            switch (addr.type)
            {
            case Sample::WAV_FILE:
            case Sample::FLAC_FILE:
            case Sample::MP3_FILE:
            case Sample::AIFF_FILE:
            {
                loadSampleByPathToID(addr.path, id);
            }
            break;
            case Sample::SF2_FILE:
            {
                loadSampleFromSF2ToID(addr.path, nullptr, addr.instrument, addr.region, id);
            }
            break;
            case Sample::MULTISAMPLE_FILE:
            {
                loadSampleFromMultiSample(addr.path, addr.region, id);
            }
            break;
            }
        }
    }
}

SampleManager::~SampleManager() { SCLOG("Destroying Sample Manager"); }

std::optional<SampleID> SampleManager::loadSampleByPath(const fs::path &p)
{
    return loadSampleByPathToID(p, SampleID::next());
}

std::optional<SampleID> SampleManager::loadSampleByPathToID(const fs::path &p, const SampleID &id)
{
    SCLOG("Loading sample " << p.u8string() << " into " << id.to_string());
    assert(threadingChecker.isSerialThread());
    SampleID::guaranteeNextAbove(id);

    for (const auto &[id, sm] : samples)
    {
        if (sm->getPath() == p)
        {
            return id;
        }
    }

    auto sp = std::make_shared<Sample>(id);

    if (!sp->load(p))
    {
        return std::nullopt;
    }

    samples[sp->id] = sp;
    return sp->id;
}

std::optional<SampleID> SampleManager::loadSampleFromSF2(const fs::path &p, sf2::File *f,
                                                         int instrument, int region)
{
    return loadSampleFromSF2ToID(p, f, instrument, region, SampleID::next());
}

std::optional<SampleID> SampleManager::loadSampleFromSF2ToID(const fs::path &p, sf2::File *f,
                                                             int instrument, int region,
                                                             const SampleID &sid)
{
    if (!f)
    {
        if (sf2FilesByPath.find(p.u8string()) == sf2FilesByPath.end())
        {
            try
            {
                SCLOG("Opening file " << p.u8string());

                auto riff = std::make_unique<RIFF::File>(p.u8string());
                auto sf = std::make_unique<sf2::File>(riff.get());
                sf2FilesByPath[p.u8string()] = {std::move(riff), std::move(sf)};
            }
            catch (RIFF::Exception e)
            {
                return {};
            }
        }
        f = sf2FilesByPath[p.u8string()].second.get();
    }

    assert(f);
    for (const auto &[id, sm] : samples)
    {
        if (sm->type == Sample::SF2_FILE)
        {
            const auto &[type, path, inst, reg] = sm->getSampleFileAddress();
            if (path == p && instrument == inst && region == reg)
                return id;
        }
    }

    auto sp = std::make_shared<Sample>(sid);

    SCLOG("Loading individual sf2 sample " << SCD(instrument) << SCD(region) << SCD(p.u8string()));
    if (!sp->loadFromSF2(p, f, instrument, region))
        return {};

    samples[sp->id] = sp;
    return sp->id;
}

std::optional<SampleID> SampleManager::setupSampleFromMultifile(const fs::path &p, int idx,
                                                                void *data, size_t dataSize)
{
    auto sid = SampleID::next();
    auto sp = std::make_shared<Sample>(sid);

    sp->parse_riff_wave(data, dataSize);
    sp->type = Sample::MULTISAMPLE_FILE;
    sp->region = idx;
    sp->mFileName = p;
    samples[sp->id] = sp;
    return sp->id;
}

std::optional<SampleID> SampleManager::loadSampleFromMultiSample(const fs::path &p, int idx,
                                                                 const SampleID &id)
{
    if (zipArchives.find(p.u8string()) == zipArchives.end())
    {
        zipArchives[p.u8string()] = std::make_unique<ZipArchiveHolder>(p);
    }
    const auto &za = zipArchives[p.u8string()];
    if (!za->isOpen)
        return std::nullopt;

    auto sp = std::make_shared<Sample>(id);

    size_t ssize;
    auto data = mz_zip_reader_extract_to_heap(&za->zip_archive, idx, &ssize, 0);

    sp->parse_riff_wave(data, ssize);
    sp->type = Sample::MULTISAMPLE_FILE;
    sp->region = idx;
    sp->mFileName = p;
    samples[sp->id] = sp;

    free(data);

    return sp->id;
}

void SampleManager::purgeUnreferencedSamples()
{
    auto preSize{samples.size()};
    auto b = samples.begin();
    while (b != samples.end())
    {
        auto ct = b->second.use_count();
        if (ct <= 1)
        {
            SCLOG("Purging sample " << b->first.to_string() << " from "
                                    << b->second->mFileName.u8string())
            b = samples.erase(b);
        }
        else
        {
            b++;
        }
    }

    if (samples.size() != preSize)
    {
        SCLOG_WFUNC("PostPurge : Purged " << (preSize - samples.size()));
    }
}
} // namespace scxt::sample
