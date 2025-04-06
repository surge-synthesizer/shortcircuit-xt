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
#include "configuration.h"
#include "sample_manager.h"
#include "infrastructure/md5support.h"

namespace scxt::sample
{

void SampleManager::restoreFromSampleAddressesAndIDs(const sampleAddressesAndIds_t &r)
{
    fs::path relativeMarker{relativeSentinel};
    for (const auto &[id, origAddr] : r)
    {
        Sample::SampleFileAddress addr = origAddr;
        // Handle relative paths
        if (!addr.path.empty())
        {
            auto top = addr.path.begin();
            if (*top == relativeMarker && !relativeRoot.empty())
            {
                auto pt = relativeRoot;
                ++top;
                while (top != addr.path.end())
                    pt /= *top++;
                addr.path = pt;
            }
        }

        // handle monolith redirects
        auto mit = monolithIndex.find(addr.path);
        if (mit != monolithIndex.end())
        {
            assert(!monolithPath.empty());
            SCLOG_IF(sampleLoadAndPurge, "Restoring monolith at " << addr.path.u8string());
            addr = {Sample::SourceType::SCXT_FILE, monolithPath, "", -1, -1, mit->second};
        }

        if (!fs::exists(addr.path))
        {
            addSampleAsMissing(id, addr);
        }
        else
        {
            auto nid = loadSampleByFileAddress(addr, id);

            if (nid.has_value())
            {
                if (*nid != id)
                {
                    addIdAlias(id, *nid);
                }
            }
        }
    }
}

SampleManager::~SampleManager() {}

std::optional<SampleID>
SampleManager::loadSampleByFileAddress(const Sample::SampleFileAddress &addr, const SampleID &id)
{
    std::optional<SampleID> nid;
    switch (addr.type)
    {
    case Sample::WAV_FILE:
    case Sample::FLAC_FILE:
    case Sample::MP3_FILE:
    case Sample::AIFF_FILE:
    {
        nid = loadSampleByPath(addr.path);
    }
    break;
    case Sample::SF2_FILE:
    {
        nid = loadSampleFromSF2(addr.path, nullptr, addr.preset, addr.instrument, addr.region);
    }
    break;
    case Sample::MULTISAMPLE_FILE:
    {
        nid = loadSampleFromMultiSample(addr.path, addr.region, id);
    }
    break;
    case Sample::GIG_FILE:
    {
        nid = loadSampleFromGIG(addr.path, nullptr, addr.preset, addr.instrument, addr.region);
    }
    break;
    case Sample::SCXT_FILE:
    {
        nid = loadSampleFromSCXTMonolith(addr.path, nullptr, addr.preset, addr.instrument,
                                         addr.region);
    }
    break;
    case Sample::SFZ_FILE:
    {
        SCLOG("How did this happen?");
    }
    break;
        // add something here? don't forget to add it in the multi resolver too in
        // resolveSingleFileMissingWorkItem
    }

    return nid;
}

std::optional<SampleID> SampleManager::loadSampleByPath(const fs::path &p)
{
    assert(threadingChecker.isSerialThread());

    for (const auto &[alreadyId, sm] : samples)
    {
        if (sm->getPath() == p)
        {
            return alreadyId;
        }
    }

    auto sp = std::make_shared<Sample>();

    if (!sp->load(p))
    {
        SCLOG("Failed to load sample from '" << p.u8string() << "'");
        return std::nullopt;
    }

    samples[sp->id] = sp;
    SCLOG_IF(sampleLoadAndPurge, "Loading : " << p.u8string());
    SCLOG_IF(sampleLoadAndPurge, "        : " << sp->id.to_string());

    updateSampleMemory();
    return sp->id;
}

std::optional<SampleID> SampleManager::loadSampleFromSF2(const fs::path &p, sf2::File *f,
                                                         int preset, int instrument, int region)
{
    if (!f)
    {
        if (sf2FilesByPath.find(p.u8string()) == sf2FilesByPath.end())
        {
            try
            {
                SCLOG_IF(sampleLoadAndPurge, "Opening SF2 : " << p.u8string());

                auto riff = std::make_unique<RIFF::File>(p.u8string());
                auto sf = std::make_unique<sf2::File>(riff.get());
                sf2FilesByPath[p.u8string()] = {std::move(riff), std::move(sf)};
            }
            catch (RIFF::Exception e)
            {
                return {};
            }
        }
        f = std::get<1>(sf2FilesByPath[p.u8string()]).get();
    }

    if (sf2MD5ByPath.find(p.u8string()) == sf2MD5ByPath.end())
        sf2MD5ByPath[p.u8string()] = infrastructure::createMD5SumFromFile(p);

    assert(f);

    auto sidx = -1;
    if (preset >= 0 && instrument >= 0)
    {
        sidx = findSF2SampleIndexFor(f, preset, instrument, region);
    }
    else
    {
        sidx = region;
    }

    if (sidx < 0 && sidx >= f->GetSampleCount())
    {
        return std::nullopt;
    }
    for (const auto &[id, sm] : samples)
    {
        if (sm->type == Sample::SF2_FILE)
        {
            if (sm->getPath() == p && sm->getCompoundRegion() == sidx)
                return id;
        }
    }

    auto sp = std::make_shared<Sample>();

    if (!sp->loadFromSF2(p, f, sidx))
        return {};

    sp->md5Sum = sf2MD5ByPath[p.u8string()];
    assert(!sp->md5Sum.empty());
    sp->id.setAsMD5WithAddress(sp->md5Sum, -1, -1, sidx);
    sp->id.setPathHash(p);

    SCLOG_IF(sampleLoadAndPurge, "Loading : " << p.u8string());
    SCLOG_IF(sampleLoadAndPurge, "        : " << sp->displayName);
    SCLOG_IF(sampleLoadAndPurge, "        : " << sp->id.to_string());

    samples[sp->id] = sp;
    updateSampleMemory();
    return sp->id;
}

int SampleManager::findSF2SampleIndexFor(sf2::File *f, int presetNum, int instrument, int region)
{
    auto *preset = f->GetPreset(presetNum);

    auto *presetRegion = preset->GetRegion(instrument);
    sf2::Instrument *instr = presetRegion->pInstrument;

    if (instr->pGlobalRegion)
    {
        // TODO: Global Region
    }

    auto sfsample = instr->GetRegion(region)->GetSample();
    if (!sfsample)
        return false;

    for (int i = 0; i < f->GetSampleCount(); ++i)
    {
        if (f->GetSample(i) == sfsample)
        {
            return i;
        }
    }
    return -1;
}

std::optional<SampleID> SampleManager::loadSampleFromGIG(const fs::path &p, gig::File *f,
                                                         int preset, int instrument, int region)
{
    if (!f)
    {
        if (gigFilesByPath.find(p.u8string()) == gigFilesByPath.end())
        {
            try
            {
                SCLOG_IF(sampleLoadAndPurge, "Opening gig : " << p.u8string());

                auto riff = std::make_unique<RIFF::File>(p.u8string());
                auto sf = std::make_unique<gig::File>(riff.get());
                gigFilesByPath[p.u8string()] = {std::move(riff), std::move(sf)};
            }
            catch (RIFF::Exception e)
            {
                return {};
            }
        }
        f = std::get<1>(gigFilesByPath[p.u8string()]).get();
    }

    if (gigMD5ByPath.find(p.u8string()) == gigMD5ByPath.end())
        gigMD5ByPath[p.u8string()] = infrastructure::createMD5SumFromFile(p);

    assert(f);

    auto sidx = region;

    if (sidx < 0 && sidx >= f->CountSamples())
    {
        return std::nullopt;
    }
    for (const auto &[id, sm] : samples)
    {
        if (sm->type == Sample::GIG_FILE)
        {
            if (sm->getPath() == p && sm->getCompoundRegion() == sidx)
                return id;
        }
    }

    auto sp = std::make_shared<Sample>();

    if (!sp->loadFromGIG(p, f, sidx))
        return {};

    sp->md5Sum = gigMD5ByPath[p.u8string()];
    assert(!sp->md5Sum.empty());
    sp->id.setAsMD5WithAddress(sp->md5Sum, -1, -1, sidx);
    sp->id.setPathHash(p);

    SCLOG_IF(sampleLoadAndPurge, "Loading : " << p.u8string());
    SCLOG_IF(sampleLoadAndPurge, "        : " << sp->displayName);
    SCLOG_IF(sampleLoadAndPurge, "        : " << sp->id.to_string());

    samples[sp->id] = sp;
    updateSampleMemory();
    return sp->id;
}

std::optional<SampleID> SampleManager::loadSampleFromSCXTMonolith(const fs::path &p, RIFF::File *f,
                                                                  int preset, int instrument,
                                                                  int region)
{
    if (!f)
    {
        if (scxtMonolithFilesByPath.find(p.u8string()) == scxtMonolithFilesByPath.end())
        {
            try
            {
                SCLOG_IF(sampleLoadAndPurge, "Opening gig : " << p.u8string());

                auto riff = std::make_unique<RIFF::File>(p.u8string());
                scxtMonolithFilesByPath[p.u8string()] = std::move(riff);
            }
            catch (RIFF::Exception e)
            {
                return {};
            }
        }
        f = scxtMonolithFilesByPath[p.u8string()].get();
    }

    if (scxtMonolithMD5ByPath.find(p.u8string()) == scxtMonolithMD5ByPath.end())
        scxtMonolithMD5ByPath[p.u8string()] = infrastructure::createMD5SumFromFile(p);

    assert(f);

    auto sidx = region;

    for (const auto &[id, sm] : samples)
    {
        if (sm->type == Sample::SCXT_FILE)
        {
            if (sm->getPath() == p && sm->getCompoundRegion() == sidx)
                return id;
        }
    }

    auto sp = std::make_shared<Sample>();

    if (!sp->loadFromSCXTMonolith(p, f, sidx))
        return {};

    sp->md5Sum = scxtMonolithMD5ByPath[p.u8string()];
    assert(!sp->md5Sum.empty());
    sp->id.setAsMD5WithAddress(sp->md5Sum, -1, -1, sidx);
    sp->id.setPathHash(p);

    SCLOG_IF(sampleLoadAndPurge, "Loading : " << p.u8string());
    SCLOG_IF(sampleLoadAndPurge, "        : " << sp->displayName);
    SCLOG_IF(sampleLoadAndPurge, "        : " << sp->id.to_string());

    samples[sp->id] = sp;
    updateSampleMemory();
    return sp->id;
}

std::optional<SampleID> SampleManager::setupSampleFromMultifile(const fs::path &p,
                                                                const std::string &md5, int idx,
                                                                void *data, size_t dataSize)
{
    auto sp = std::make_shared<Sample>();
    sp->id.setAsMD5WithAddress(md5, idx, -1, -1);
    sp->id.setPathHash(p);

    sp->parse_riff_wave(data, dataSize);
    sp->type = Sample::MULTISAMPLE_FILE;
    sp->region = idx;
    sp->mFileName = p;
    samples[sp->id] = sp;
    updateSampleMemory();
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
    updateSampleMemory();

    free(data);

    SCLOG_IF(sampleLoadAndPurge, "Loading : " << p.u8string());
    SCLOG_IF(sampleLoadAndPurge, "        : " << sp->id.to_string());

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
            SCLOG_IF(sampleLoadAndPurge, "Purging : " << b->second->mFileName.u8string());
            SCLOG_IF(sampleLoadAndPurge, "        : " << b->first.to_string());
            if (b->second->isMissingPlaceholder)
            {
                SCLOG_IF(sampleLoadAndPurge, "        : Missing Placeholder");
            }

            b = samples.erase(b);
        }
        else
        {
            b++;
        }
    }

    if (samples.size() != preSize)
    {
        SCLOG_IF(sampleLoadAndPurge, "PostPurge : Purged " << (preSize - samples.size())
                                                           << " Remaining " << samples.size());
    }
    updateSampleMemory();
}

void SampleManager::updateSampleMemory()
{
    uint64_t res = 0;
    for (const auto &[id, smp] : samples)
    {
        res += smp->sample_length * smp->channels * (smp->bitDepth == Sample::BD_I16 ? 4 : 8);
    }
    sampleMemoryInBytes = res;
}

SampleManager::sampleAddressesAndIds_t
SampleManager::getSampleAddressesFor(const std::vector<SampleID> &sids) const
{
    SampleManager::sampleAddressesAndIds_t res;
    for (const auto &sid : sids)
    {
        auto smp = getSample(sid);
        if (!smp)
        {
            SCLOG("WARNING: Requested non-existant sample at " << sid.to_string());
        }
        else
        {
            res.emplace_back(sid, smp->getSampleFileAddress());
        }
    }
    return res;
}

void SampleManager::addSampleAsMissing(const SampleID &id, const Sample::SampleFileAddress &f)
{
    if (samples.find(id) == samples.end())
    {
        auto ms = Sample::createMissingPlaceholder(f);
        ms->id = id;
        ms->id.setPathHash(f.path);

        SCLOG_IF(missingResolution, "Missing : " << f.path.u8string());
        SCLOG_IF(missingResolution, "        : " << ms->id.to_string());

        samples[ms->id] = ms;

        if (ms->id != id)
        {
            // Path change so
            addIdAlias(id, ms->id);
        }
    }
}

} // namespace scxt::sample
