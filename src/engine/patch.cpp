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

#include "patch.h"
#include "sst/basic-blocks/mechanics/block-ops.h"

namespace scxt::engine
{
void Patch::process(Engine &e)
{
    namespace mech = sst::basic_blocks::mechanics;
    // Run each of the parts, accumulating onto the engine busses
    for (const auto &part : parts)
    {
        if (part->isActive())
        {
            part->process(e);
        }
    }

    // Then process the part busses
    for (auto &b : busses.partBusses)
    {
        b.process();
        if (b.supportsSends && b.hasSends)
        {
            // TOD accumulate my sends
        }
    }

    // Process my send busses
    for (auto &b : busses.auxBusses)
        b.process();

    // And finally push onto the main bus
    for (auto [bi, br] : sst::cpputils::enumerate(busses.partToVSTRouting))
    {
        if (br == 0)
        {
            // accumulate onto main
            mech::accumulate_from_to<blockSize>(busses.partBusses[bi].output[0],
                                                busses.mainBus.output[0]);
            mech::accumulate_from_to<blockSize>(busses.partBusses[bi].output[1],
                                                busses.mainBus.output[1]);
        }
    }

    for (auto [bi, br] : sst::cpputils::enumerate(busses.auxToVSTRouting))
    {
        if (br == 0)
        {
            // accumulate onto main
            mech::accumulate_from_to<blockSize>(busses.auxBusses[bi].output[0],
                                                busses.mainBus.output[0]);
            mech::accumulate_from_to<blockSize>(busses.auxBusses[bi].output[1],
                                                busses.mainBus.output[1]);
        }
    }

    // And run the main bus
    busses.mainBus.process();
}

void Patch::setupBussesOnUnstream(Engine &e)
{
    // Assume the bus storage is correct
    busses.mainBus.initializeAfterUnstream(e);
    for (auto &p : busses.partBusses)
        p.initializeAfterUnstream(e);
    for (auto &a : busses.auxBusses)
        a.initializeAfterUnstream(e);
}
} // namespace scxt::engine