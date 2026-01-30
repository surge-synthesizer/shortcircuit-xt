/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2026, Various authors, as described in the github
 * transaction log.
 *
 * This source file and all other files in the shortcircuit-xt repo outside of
 * `libs/` are licensed under the MIT license, available in the
 * file LICENSE or at https://opensource.org/license/mit.
 *
 * As some dependencies of ShortcircuitXT are released under the GNU General
 * Public License 3, if you distribute a binary of ShortcircuitXT
 * without breaking those dependencies, the combined work must be
 * distributed under GPL3.
 *
 * ShortcircuitXT is inspired by, and shares a small amount of code with,
 * the commercial product Shortcircuit 1 and 2, released by VemberTech
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

    // Clear the busses
    busses.mainBus.clear();
    for (auto &b : busses.partBusses)
        b.clear();
    for (auto &b : busses.auxBusses)
        b.clear();

    // Run each of the parts, accumulating onto the engine busses
    for (const auto &part : parts)
    {
        if (part->isActive())
        {
            part->process(e);
        }
    }

    for (auto &b : busses.partBusses)
    {
        if (!busses.busUsed[b.address])
        {
            continue;
        }
        b.process();
        if (b.busSendStorage.supportsSends && b.busSendStorage.hasSends)
        {
            for (int i = 0; i < numAux; ++i)
            {
                if (b.busSendStorage.sendLevels[i] != 0.f)
                {
                    busses.busUsed[i + AUX_0] = true;
                    busses.auxBusses[i].inRingout = busses.auxBusses[i].inRingout && b.inRingout;
                    busses.auxBusses[i].silenceMaxUpstreamBusses += b.silenceMaxSelf;
                    switch (b.busSendStorage.auxLocation[i])
                    {
                    case Bus::BusSendStorage::PRE_FX:
                        mech::scale_accumulate_from_to<blockSize>(
                            b.auxoutputPreFX[0], b.auxoutputPreFX[1],
                            b.busSendStorage.sendLevels[i], busses.auxBusses[i].output[0],
                            busses.auxBusses[i].output[1]);
                        break;
                    case Bus::BusSendStorage::POST_FX_PRE_VCA:
                        mech::scale_accumulate_from_to<blockSize>(
                            b.auxoutputPreVCA[0], b.auxoutputPreVCA[1],
                            b.busSendStorage.sendLevels[i], busses.auxBusses[i].output[0],
                            busses.auxBusses[i].output[1]);
                        break;
                    case Bus::BusSendStorage::POST_VCA:
                        mech::scale_accumulate_from_to<blockSize>(
                            b.auxoutputPostVCA[0], b.auxoutputPostVCA[1],
                            b.busSendStorage.sendLevels[i], busses.auxBusses[i].output[0],
                            busses.auxBusses[i].output[1]);
                        break;
                    }
                }
            }
        }
    }

    // Process my send busses
    for (auto &b : busses.auxBusses)
    {
        b.process();
        busses.mainBus.silenceMaxUpstreamBusses += b.silenceMaxSelf + b.silenceMaxUpstreamBusses;
    }

    // TODO - we can be more parsimonious here if we don't use these
    memset(busses.pluginNonMainOutputs, 0, sizeof(busses.pluginNonMainOutputs));

    // And finally push onto the main bus
    for (auto [bi, br] : sst::cpputils::enumerate(busses.partToVSTRouting))
    {
        if (br == 0)
        {
            busses.mainBus.inRingout = busses.mainBus.inRingout && busses.partBusses[bi].inRingout;
            busses.mainBus.silenceMaxUpstreamBusses += busses.partBusses[bi].silenceMaxSelf;

            // accumulate onto main
            mech::accumulate_from_to<blockSize>(busses.partBusses[bi].output[0],
                                                busses.mainBus.output[0]);
            mech::accumulate_from_to<blockSize>(busses.partBusses[bi].output[1],
                                                busses.mainBus.output[1]);
        }
        else
        {
            mech::accumulate_from_to<blockSize>(busses.partBusses[bi].output[0],
                                                busses.pluginNonMainOutputs[br - 1][0]);
            mech::accumulate_from_to<blockSize>(busses.partBusses[bi].output[1],
                                                busses.pluginNonMainOutputs[br - 1][1]);
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
        else
        {
            mech::accumulate_from_to<blockSize>(busses.auxBusses[bi].output[0],
                                                busses.pluginNonMainOutputs[br - 1][0]);
            mech::accumulate_from_to<blockSize>(busses.auxBusses[bi].output[1],
                                                busses.pluginNonMainOutputs[br - 1][1]);
        }
    }

    // And run the main bus
    busses.mainBus.process();
}

void Patch::setupPatchOnUnstream(Engine &e)
{
    // Assume the bus storage is correct
    busses.mainBus.initializeAfterUnstream(e);
    busses.mainBus.busSendStorage.supportsSends = false;
    for (auto &p : busses.partBusses)
    {
        p.initializeAfterUnstream(e);
        p.busSendStorage.supportsSends = true;
    }
    for (auto &a : busses.auxBusses)
    {
        a.busSendStorage.supportsSends = false;
        a.initializeAfterUnstream(e);
    }
}
void Patch::onSampleRateChanged()
{
    for (const auto &part : parts)
        part->setSampleRate(samplerate);

    busses.mainBus.setSampleRate(getSampleRate());
    for (auto &p : busses.partBusses)
    {
        p.setSampleRate(getSampleRate());
    }
    for (auto &a : busses.auxBusses)
    {
        a.setSampleRate(getSampleRate());
    }
}
} // namespace scxt::engine