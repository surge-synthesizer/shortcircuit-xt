//
// Created by Paul Walker on 2/1/23.
//

#include "part.h"
#include "vembertech/vt_dsp/basic_dsp.h"

namespace scxt::engine
{
void Part::process()
{
    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));

    for (const auto &g : groups)
    {
        if (g->isActive())
        {
            g->process();
            for (int i = 0; i < g->getNumOutputs(); ++i)
            {
                accumulate_block(g->output[i][0], output[i][0], blockSizeQuad);
                accumulate_block(g->output[i][1], output[i][1], blockSizeQuad);
            }
        }
    }
}
} // namespace scxt::engine