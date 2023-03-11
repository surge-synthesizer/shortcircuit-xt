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

std::optional<SampleID> SampleManager::loadSampleFromSF2(const fs::path &p, sf2::File *f, int instrument, int region)
{
    return loadSampleFromSF2ToID(p, f, instrument, region, SampleID::next());
}


std::optional<SampleID> SampleManager::loadSampleFromSF2ToID(const fs::path &p, sf2::File *f,
                                                             int instrument, int region,
                                                             const SampleID &sid)
{
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

    if (!sp->loadFromSF2(f, instrument, region))
        return {};

    samples[sp->id] = sp;
    return sp->id;
}
} // namespace scxt::sample
