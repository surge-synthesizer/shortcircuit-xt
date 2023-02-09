//
// Created by Paul Walker on 2/1/23.
//

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