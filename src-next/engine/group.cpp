//
// Created by Paul Walker on 2/1/23.
//

#include "group.h"
#include "part.h"
#include <cassert>
#include "vembertech/vt_dsp/basic_dsp.h"

namespace scxt::engine
{

void Group::process()
{
    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));

    for (const auto &z : zones)
    {
        if (z->isActive())
        {
            z->process();
            for (int i = 0; i < z->getNumOutputs(); ++i)
            {
                accumulate_block(z->output[i][0], output[i][0], blockSizeQuad);
                accumulate_block(z->output[i][1], output[i][1], blockSizeQuad);
            }
        }
    }
}

void Group::addActiveZone()
{
    if (activeZones == 0)
    {
        parentPart->addActiveGroup();
    }
    activeZones++;
}

void Group::removeActiveZone()
{
    assert(activeZones);
    activeZones--;
    if (activeZones == 0)
    {
        parentPart->removeActiveGroup();
    }
}
} // namespace scxt::engine