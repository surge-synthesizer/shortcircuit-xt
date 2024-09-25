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

#include "missing_resolution.h"

namespace scxt::engine
{
std::vector<MissingResolutionWorkItem> collectMissingResolutionWorkItems(const Engine &e)
{
    std::vector<MissingResolutionWorkItem> res;

    int pidx{0};
    for (const auto &p : *(e.getPatch()))
    {
        int gidx{0};
        for (const auto &g : *p)
        {
            int zidx{0};
            for (const auto &z : *g)
            {
                int idx{0};
                for (const auto &v : z->variantData.variants)
                {
                    if (v.active && z->samplePointers[idx]->isMissingPlaceholder)
                    {
                        auto &sm = z->samplePointers[idx];
                        MissingResolutionWorkItem wf;
                        wf.address = {pidx, gidx, zidx};
                        wf.variant = idx;
                        wf.path = sm->mFileName;
                        wf.md5sum = sm->md5Sum;
                        res.push_back(wf);
                    }
                    idx++;
                }
                zidx++;
            }
            gidx++;
        }
        pidx++;
    }

    return res;
}
} // namespace scxt::engine