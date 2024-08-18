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

#include "selection/selection_manager.h"

#include "infrastructure/sse_include.h"
#include "sst/basic-blocks/mechanics/block-ops.h"

namespace scxt::engine
{
void Part::process(Engine &e)
{
    namespace blk = sst::basic_blocks::mechanics;

    for (auto &sm : midiCCSmoothers)
        if (sm.active)
            sm.step();
    pitchBendSmoother.step();

    for (const auto &g : groups)
    {
        if (g->isActive())
        {
            g->process(e);

            auto bi = g->outputInfo.routeTo;
            if (bi == DEFAULT_BUS)
            {
                // this should be the route to point
                bi = (BusAddress)(PART_0 + partNumber);
            }
            auto &obus = e.getPatch()->busses.busByAddress(bi);

            blk::accumulate_from_to<blockSize>(g->output[0], obus.output[0]);
            blk::accumulate_from_to<blockSize>(g->output[1], obus.output[1]);
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
            auto data =
                zoneMappingItem_t{z->mapping.keyboardRange, z->mapping.velocityRange, z->getName()};
            auto addr = selection::SelectionManager::ZoneAddress(pidx, gidx, zidx);
            // res[addr] = data;
            res.emplace_back(addr, data);
            zidx++;
        }
        gidx++;
    }
    return res;
}
} // namespace scxt::engine