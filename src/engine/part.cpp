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

#include "part.h"
#include "bus.h"
#include "patch.h"
#include "engine.h"
#include "feature_enums.h"

#include "selection/selection_manager.h"

#include "sst/basic-blocks/simd/setup.h"
#include "sst/basic-blocks/mechanics/block-ops.h"
#include "sst/basic-blocks/dsp/PanLaws.h"

namespace scxt::engine
{
void Part::process(Engine &e)
{
    namespace blk = sst::basic_blocks::mechanics;

    float lcp alignas(16)[2][blockSize];

    externalSignalLag.processAll();

    auto lev = configuration.level;
    lev = lev * lev * lev;

    namespace pl = sst::basic_blocks::dsp::pan_laws;
    auto pan = configuration.pan;
    pl::panmatrix_t pmat{1, 1, 0, 0};
    if (pan != 0)
        pl::stereoEqualPower(0.5 * (pan + 1), pmat);

    for (const auto &g : groups)
    {
        if (g->isActive())
        {
            g->process(e);

            auto bi = g->outputInfo.routeTo;
            if (bi == DEFAULT_BUS)
            {
                // this should be the route to point
                if (configuration.routeTo == DEFAULT_BUS)
                    bi = (BusAddress)(PART_0 + partNumber);
                else
                    bi = configuration.routeTo;

                blk::mul_block<blockSize>(g->output[0], lev, lcp[0]);
                blk::mul_block<blockSize>(g->output[1], lev, lcp[1]);

                if (pan != 0.0)
                {
                    for (int i = 0; i < blockSize; ++i)
                    {
                        auto il = lcp[0][i];
                        auto ir = lcp[1][i];
                        lcp[0][i] = pmat[0] * il + pmat[2] * ir;
                        lcp[1][i] = pmat[1] * ir + pmat[3] * il;
                    }
                }

                auto &obus = e.getPatch()->busses.busByAddress(bi);

                blk::accumulate_from_to<blockSize>(lcp[0], obus.output[0]);
                blk::accumulate_from_to<blockSize>(lcp[1], obus.output[1]);
            }
            else
            {

                auto &obus = e.getPatch()->busses.busByAddress(bi);

                blk::accumulate_from_to<blockSize>(g->output[0], obus.output[0]);
                blk::accumulate_from_to<blockSize>(g->output[1], obus.output[1]);
            }
        }
    }
}

Part::zoneMappingSummary_t Part::getZoneMappingSummary()
{
    zoneMappingSummary_t res;

    int pidx{partNumber};
    int gidx{0};
    for (const auto &g : groups)
    {
        int zidx{0};
        for (const auto &z : *g)
        {
            // get address for zone
            auto addr = selection::SelectionManager::ZoneAddress(pidx, gidx, zidx);
            int32_t features{0};
            if (z->missingSampleCount() > 0)
            {
                features |= ZoneFeatures::MISSING_SAMPLE;
            }
            auto data = zoneMappingItem_t{addr, z->mapping.keyboardRange, z->mapping.velocityRange,
                                          z->getName(), features};
            // res[addr] = data;
            res.emplace_back(data);
            zidx++;
        }
        gidx++;
    }
    return res;
}

std::vector<SampleID> Part::getSamplesUsedByPart() const
{
    std::unordered_set<SampleID> resSet;
    for (const auto &g : groups)
    {
        for (const auto &z : g->getZones())
        {
            for (const auto &var : z->variantData.variants)
            {
                if (var.sampleID.isValid())
                {
                    resSet.insert(var.sampleID);
                }
            }
        }
    }
    return std::vector<SampleID>(resSet.begin(), resSet.end());
}
} // namespace scxt::engine