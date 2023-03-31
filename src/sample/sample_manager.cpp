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
            {
                loadSampleByPathToID(addr.path, id);
            }
            break;
            case Sample::SF2_FILE:
            {
                loadSampleFromSF2ToID(addr.path, nullptr, addr.instrument, addr.region, id);
            }
            break;
            }
        }
    }
}

std::optional<SampleID> SampleManager::loadSampleByPath(const fs::path &p)
{
    return loadSampleByPathToID(p, SampleID::next());
}

std::optional<SampleID> SampleManager::loadSampleByPathToID(const fs::path &p, const SampleID &id)
{
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
        return {};

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
                SCDBGCOUT << "Opening file " << p.u8string() << std::endl;

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

    if (!sp->loadFromSF2(p, f, instrument, region))
        return {};

    samples[sp->id] = sp;
    return sp->id;
}
} // namespace scxt::sample
