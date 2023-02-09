//
// Created by Paul Walker on 2/1/23.
//

#include "group.h"
#include "part.h"
#include "sst/basic-blocks/mechanics/block-ops.h"

#include <cassert>

namespace scxt::engine
{

void Group::process()
{
    namespace blk = sst::basic_blocks::mechanics;

    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));

    for (const auto &z : zones)
    {
        if (z->isActive())
        {
            z->process();
            for (int i = 0; i < z->getNumOutputs(); ++i)
            {
                blk::accumulate_from_to<blockSize>(z->output[i][0], output[i][0]);
                blk::accumulate_from_to<blockSize>(z->output[i][1], output[i][1]);
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