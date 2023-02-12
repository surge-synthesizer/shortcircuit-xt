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

#include "part.h"
#include "sst/basic-blocks/mechanics/block-ops.h"

namespace scxt::engine
{
void Part::process()
{
    namespace blk = sst::basic_blocks::mechanics;

    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));

    for (const auto &g : groups)
    {
        if (g->isActive())
        {
            g->process();
            for (int i = 0; i < g->getNumOutputs(); ++i)
            {
                blk::accumulate_from_to<blockSize>(g->output[i][0], output[i][0]);
                blk::accumulate_from_to<blockSize>(g->output[i][1], output[i][1]);
            }
        }
    }
}
} // namespace scxt::engine