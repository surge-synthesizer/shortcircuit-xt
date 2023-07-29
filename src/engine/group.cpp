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

#include "group.h"
#include "bus.h"
#include "part.h"

#include "infrastructure/sse_include.h"

#include "sst/basic-blocks/mechanics/block-ops.h"

#include <cassert>

#include "messaging/messaging.h"
#include "patch.h"
#include "engine.h"
#include "group_and_zone_impl.h"

namespace scxt::engine
{

void Group::process(Engine &e)
{
    namespace blk = sst::basic_blocks::mechanics;

    // TODO these memsets are probably gratuitous
    memset(output, 0, sizeof(output));

    for (const auto &z : zones)
    {
        if (z->isActive())
        {
            z->process(e);
            if (routeTo == DEFAULT_BUS)
            {
                blk::accumulate_from_to<blockSize>(z->output[0], output[0]);
                blk::accumulate_from_to<blockSize>(z->output[1], output[1]);
            }
            else
            {
                assert(false);
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

engine::Engine *Group::getEngine()
{
    if (parentPart && parentPart->parentPatch)
        return parentPart->parentPatch->parentEngine;
    return nullptr;
}

void Group::setupOnUnstream(const engine::Engine &e)
{
    for (int p = 0; p < processorCount; ++p)
    {
        setupProcessorControlDescriptions(p, processorStorage[p].type);
        onProcessorTypeChanged(p, processorStorage[p].type);
    }
}

template struct HasGroupZoneProcessors<Group>;
} // namespace scxt::engine