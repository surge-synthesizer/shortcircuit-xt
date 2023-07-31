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

    // When we have alternate group routing we change these pointers
    assert(routeTo == DEFAULT_BUS);
    float *lOut = output[0];
    float *rOut = output[1];

    modMatrix.updateModulatorUsed(*this);
    modMatrix.copyBaseValuesFromGroup(*this);
    modMatrix.initializeModulationValues();

    if (anyModulatorUsed)
    {
        bool gated{false};
        for (const auto &z : zones)
        {
            gated = gated || (z->gatedVoiceCount > 0);
        }

        for (int i = 0; i < egPerGroup; ++i)
        {
            if (!gegUsed[i])
                continue;
            auto &eg = gegEvaluators[i];
            auto &egs = gegStorage[i];
            if (gated && eg.stage > ahdsrenv_t::s_hold)
            {
                eg.attackFrom(eg.outBlock0);
            }
            // fixme - put these all in the mod matrix
            eg.processBlock(egs.a, egs.h, egs.d, egs.s, egs.r, egs.aShape, egs.dShape, egs.rShape,
                            gated);
        }

        // for LFO
        // -- if matrix consumes lfo
        // ---- process}

        modMatrix.process();
    }

    for (const auto &z : zones)
    {
        if (z->isActive())
        {
            z->process(e);
            blk::accumulate_from_to<blockSize>(z->output[0], lOut);
            blk::accumulate_from_to<blockSize>(z->output[1], rOut);
        }
    }

    // for processor do the necessary

    // multiply by vca level from matrix
    auto mlev = modMatrix.getValue(modulation::gmd_grouplevel, 0);
    levelBlk.set_target(mlev);
    levelBlk.multiply_2_blocks(lOut, rOut);
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