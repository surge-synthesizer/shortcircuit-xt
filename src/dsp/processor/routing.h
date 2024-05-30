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

#ifndef SCXT_SRC_DSP_PROCESSOR_ROUTING_H
#define SCXT_SRC_DSP_PROCESSOR_ROUTING_H

#include "processor.h"

namespace scxt::dsp::processor
{
template <bool OS, int N, typename Mix, typename Endpoints>
void processSequential(float fpitch, Processor *processors[engine::processorCount],
                       bool processorConsumesMono[engine::processorCount], Mix &mix,
                       Endpoints *endpoints, bool &chainIsMono, float output[2][N])
{
    namespace mech = sst::basic_blocks::mechanics;

    float tempbuf alignas(16)[2][BLOCK_SIZE << 1];

    for (auto i = 0; i < engine::processorCount; ++i)
    {
        if (processors[i])
        {
            endpoints->processorTarget[i].snapValues();
            // TODO: Snap int values here (and same in group)

            mix[i].set_target(*endpoints->processorTarget[i].mixP);

            if (chainIsMono && processorConsumesMono[i] &&
                !processors[i]->monoInputCreatesStereoOutput())
            {
                // mono to mono
                processors[i]->process_monoToMono(output[0], tempbuf[0], fpitch);
                mix[i].fade_blocks(output[0], tempbuf[0], output[0]);
            }
            else if (chainIsMono && processorConsumesMono[i])
            {
                assert(processors[i]->monoInputCreatesStereoOutput());
                // mono to stereo. process then toggle
                processors[i]->process_monoToStereo(output[0], tempbuf[0], tempbuf[1], fpitch);

                // this out[0] is NOT a typo. Input is mono
                // And this order matters. Have to splat [1] first to keep [0] around
                mix[i].fade_blocks(output[0], tempbuf[1], output[1]);
                mix[i].fade_blocks(output[0], tempbuf[0], output[0]);

                chainIsMono = false;
            }
            else if (chainIsMono)
            {
                assert(!processorConsumesMono[i]);
                // stereo to stereo. copy L to R then process
                mech::copy_from_to<blockSize << (OS ? 1 : 0)>(output[0], output[1]);
                chainIsMono = false;
                processors[i]->process_stereo(output[0], output[1], tempbuf[0], tempbuf[1], fpitch);
                mix[i].fade_blocks(output[0], tempbuf[0], output[0]);
                mix[i].fade_blocks(output[1], tempbuf[1], output[1]);
            }
            else
            {
                // stereo to stereo. process
                processors[i]->process_stereo(output[0], output[1], tempbuf[0], tempbuf[1], fpitch);
                mix[i].fade_blocks(output[0], tempbuf[0], output[0]);
                mix[i].fade_blocks(output[1], tempbuf[1], output[1]);
            }
            // TODO: What was the filter_modout? Seems SC2 never finished it
            /*
            filter_modout[0] = voice_filter[0]->modulation_output;
                                   */
        }
    }
}
} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_ROUTING_H
