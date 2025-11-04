/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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

#include "missing_resolution.h"
#include "messaging/messaging.h"

namespace scxt::engine
{
std::vector<MissingResolutionWorkItem> collectMissingResolutionWorkItems(const Engine &e)
{
    std::vector<MissingResolutionWorkItem> res;
    const auto &sm = e.getSampleManager();
    auto it = sm->samplesBegin();
    while (it != sm->samplesEnd())
    {
        const auto &[id, s] = *it;
        if (s->isMissingPlaceholder)
        {
            res.push_back({id, s->mFileName, false});
        }
        it++;
    }

    std::sort(res.begin(), res.end(),
              [](const auto &a, const auto &b) { return a.path.u8string() < b.path.u8string(); });

    auto rit = 0;
    auto rnext = 1;
    while (rnext < res.size())
    {
        if (res[rit].missingID.sameMD5As(res[rnext].missingID) && res[rit].path == res[rnext].path)
        {
            res[rit].isMultiUsed = true;
            res.erase(res.begin() + rnext);
        }
        else
        {
            rnext++;
            rit++;
        }
    }

    return res;
}

void resolveSingleFileMissingWorkItem(engine::Engine &e, const MissingResolutionWorkItem &mwi,
                                      const fs::path &p)
{

    auto smp = e.getSampleManager()->loadSampleByPath(p);
    SCLOG("Resolving : " << p.u8string());
    SCLOG("      Was : " << mwi.missingID.to_string());
    SCLOG("       To : " << smp->to_string());
    if (smp.has_value())
    {
        for (auto &p : *(e.getPatch()))
        {
            for (auto &g : *p)
            {
                for (auto &z : *g)
                {
                    int vidx{0};
                    for (auto &v : z->variantData.variants)
                    {
                        if (v.active && v.sampleID == mwi.missingID)
                        {
                            SCLOG("     Zone : " << z->getName());
                            v.sampleID = *smp;
                            z->attachToSampleAtVariation(
                                *(e.getSampleManager()), *smp, vidx,
                                engine::Zone::SampleInformationRead::ENDPOINTS);
                        }
                        vidx++;
                    }
                }
            }
        }
    }
}

void resolveMultiFileMissingWorkItem(engine::Engine &e, const MissingResolutionWorkItem &mwi,
                                     const fs::path &p)
{
    bool isSF2{false}, isMultiSample{false};
    if (extensionMatches(p, ".sf2"))
    {
        isSF2 = true;
    }

    if (!isSF2 && !isMultiSample)
    {
        SCLOG("Cant deteremine multi style for " << p.u8string());
        e.getMessageController()->reportErrorToClient(
            "Unable to resolve missing with multifile",
            "Can't determine the multifile type for path '" + p.u8string() + "''");
        return;
    }

    for (auto &pt : *(e.getPatch()))
    {
        for (auto &g : *pt)
        {
            for (auto &z : *g)
            {
                int vidx{0};
                for (auto &v : z->variantData.variants)
                {
                    if (v.active && v.sampleID.sameMD5As(mwi.missingID))
                    {
                        std::optional<SampleID> nid;
                        if (isSF2)
                        {
                            nid = e.getSampleManager()->loadSampleFromSF2(
                                p, nullptr, v.sampleID.multiAddress[0], v.sampleID.multiAddress[1],
                                v.sampleID.multiAddress[2]);
                        }
                        if (isMultiSample)
                        {
                            nid = e.getSampleManager()->loadSampleFromMultiSample(
                                p, v.sampleID.multiAddress[2], mwi.missingID);
                        }
                        if (nid.has_value())
                        {
                            SCLOG("Re-attaching to multi in " << z->getName());
                            SCLOG("      Was : " << v.sampleID.to_string());
                            SCLOG("       To : " << nid->to_string());
                            v.sampleID = *nid;
                            z->attachToSampleAtVariation(
                                *(e.getSampleManager()), *nid, vidx,
                                engine::Zone::SampleInformationRead::ENDPOINTS);
                        }
                    }
                    vidx++;
                }
            }
        }
    }
}

} // namespace scxt::engine