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

#include "sst/basic-blocks/mechanics/block-ops.h"

#include "processor.h"

namespace scxt::dsp::processor
{

template <bool OS, bool forceStereo, int N, typename Mix, typename Endpoints>
inline void runSingleProcessor(int i, float fpitch, Processor *processors[engine::processorCount],
                               bool processorConsumesMono[engine::processorCount], Mix &mix,
                               Endpoints *endpoints, bool &chainIsMono, float input[2][N],
                               float output[2][N])
{
    namespace mech = sst::basic_blocks::mechanics;

    float tempbuf alignas(16)[2][N];

    endpoints->processorTarget[i].snapValues();

    auto mixl = *endpoints->processorTarget[i].mixP;
    if (processors[i]->getType() == proct_none)
        mixl = 1.0;
    mix[i].set_target(mixl);

    if constexpr (forceStereo)
    {
        processors[i]->process_stereo(input[0], input[1], tempbuf[0], tempbuf[1], fpitch);
        mix[i].fade_blocks(input[0], tempbuf[0], output[0]);
        mix[i].fade_blocks(input[1], tempbuf[1], output[1]);
    }
    else
    {
        if (chainIsMono && processorConsumesMono[i] &&
            !processors[i]->monoInputCreatesStereoOutput())
        {
            // mono to mono
            processors[i]->process_monoToMono(input[0], tempbuf[0], fpitch);
            mix[i].fade_blocks(input[0], tempbuf[0], input[0]);
        }
        else if (chainIsMono && processorConsumesMono[i])
        {
            assert(processors[i]->monoInputCreatesStereoOutput());
            // mono to stereo. process then toggle
            processors[i]->process_monoToStereo(input[0], tempbuf[0], tempbuf[1], fpitch);

            // this out[0] is NOT a typo. Input is mono
            // And this order matters. Have to splat [1] first to keep [0] around
            mix[i].fade_blocks(input[0], tempbuf[1], output[1]);
            mix[i].fade_blocks(input[0], tempbuf[0], output[0]);

            chainIsMono = false;
        }
        else if (chainIsMono)
        {
            assert(!processorConsumesMono[i]);
            // stereo to stereo. copy L to R then process
            mech::copy_from_to<blockSize << (OS ? 1 : 0)>(input[0], input[1]);
            chainIsMono = false;
            processors[i]->process_stereo(input[0], input[1], tempbuf[0], tempbuf[1], fpitch);
            mix[i].fade_blocks(input[0], tempbuf[0], output[0]);
            mix[i].fade_blocks(input[1], tempbuf[1], output[1]);
        }
        else
        {
            // stereo to stereo. process
            processors[i]->process_stereo(input[0], input[1], tempbuf[0], tempbuf[1], fpitch);
            mix[i].fade_blocks(input[0], tempbuf[0], output[0]);
            mix[i].fade_blocks(input[1], tempbuf[1], output[1]);
        }
    }
    // TODO: What was the filter_modout? Seems SC2 never finished it
    /*
    filter_modout[0] = voice_filter[0]->modulation_output;
                           */
}

// -> 1 -> 2 -> 3 -> 4 ->
template <bool OS, bool forceStereo, int N, typename Mix, typename Endpoints>
inline void processSequential(float fpitch, Processor *processors[engine::processorCount],
                              bool processorConsumesMono[engine::processorCount], Mix &mix,
                              Endpoints *endpoints, bool &chainIsMono, float output[2][N])
{
    namespace mech = sst::basic_blocks::mechanics;

    for (auto i = 0; i < engine::processorCount; ++i)
    {
        if (processors[i])
        {
            runSingleProcessor<OS, forceStereo>(i, fpitch, processors, processorConsumesMono, mix,
                                                endpoints, chainIsMono, output, output);
        }
    }
}

// -> { 1 | 2 } -> { 3 | 4 } ->

// All in parallel
template <bool OS, bool forceStereo, int N, typename Mix, typename Endpoints>
void processPar1Pattern(float fpitch, Processor *processors[engine::processorCount],
                        bool processorConsumesMono[engine::processorCount], Mix &mix,
                        Endpoints *endpoints, bool &chainIsMono, float output[2][N])
{
    namespace mech = sst::basic_blocks::mechanics;

    float tmpbuf alignas(16)[engine::processorCount][2][N];
    bool isMute[engine::processorCount]{};

    // The mono handling here is tricky so for now just force stereo. Fix this obvs
    if (chainIsMono)
    {
        chainIsMono = false;
        mech::copy_from_to<blockSize << OS>(output[0], output[1]);
    }

    int numUnMuted = 0;

    for (int i = 0; i < engine::processorCount; ++i)
    {
        if (processors[i])
        {
            isMute[i] = processors[i]->isMuteProcessor();
            runSingleProcessor<OS, true>(i, fpitch, processors, processorConsumesMono, mix,
                                         endpoints, chainIsMono, output, tmpbuf[i]);
        }
        else
        {
            isMute[i] = false;
            mech::copy_from_to<blockSize << OS>(output[0], tmpbuf[i][0]);
            mech::copy_from_to<blockSize << OS>(output[1], tmpbuf[i][1]);
        }
        numUnMuted += (isMute[i] ? 0 : 1);
    }

    mech::clear_block<blockSize << OS>(output[0]);
    mech::clear_block<blockSize << OS>(output[1]);
    if (numUnMuted == 0)
    {
        // special case - we can't scale and don't have anything else to do
    }
    else
    {
        float scale = 1.f / numUnMuted;
        for (int i = 0; i < engine::processorCount; ++i)
        {
            if (!isMute[i])
            {
                mech::scale_accumulate_from_to<blockSize << OS>(tmpbuf[i][0], scale, output[0]);
                mech::scale_accumulate_from_to<blockSize << OS>(tmpbuf[i][1], scale, output[1]);
            }
        }
    }
}

// -> { { 1->2} | { 3->4 } ->
template <bool OS, bool forceStereo, int N, typename Mix, typename Endpoints>
void processPar2Pattern(float fpitch, Processor *processors[engine::processorCount],
                        bool processorConsumesMono[engine::processorCount], Mix &mix,
                        Endpoints *endpoints, bool &chainIsMono, float output[2][N])
{
    namespace mech = sst::basic_blocks::mechanics;

    float tempbuf12 alignas(16)[2][BLOCK_SIZE << 1];
    float tempbuf34 alignas(16)[2][BLOCK_SIZE << 1];

    bool mono12 = chainIsMono;
    bool mono34 = chainIsMono;

    bool muteIn12 = false;
    bool muteIn34 = false;

    mech::copy_from_to<scxt::blockSize << OS>(output[0], tempbuf12[0]);
    mech::copy_from_to<scxt::blockSize << OS>(output[1], tempbuf12[1]);

    if (processors[0])
    {
        muteIn12 |= processors[0]->isMuteProcessor();
        runSingleProcessor<OS, forceStereo>(0, fpitch, processors, processorConsumesMono, mix,
                                            endpoints, mono12, tempbuf12, tempbuf12);
    }
    if (processors[1])
    {
        muteIn12 |= processors[1]->isMuteProcessor();
        runSingleProcessor<OS, forceStereo>(1, fpitch, processors, processorConsumesMono, mix,
                                            endpoints, mono12, tempbuf12, tempbuf12);
    }

    mech::copy_from_to<scxt::blockSize << OS>(output[0], tempbuf34[0]);
    mech::copy_from_to<scxt::blockSize << OS>(output[1], tempbuf34[1]);

    if (processors[2])
    {
        muteIn34 |= processors[2]->isMuteProcessor();
        runSingleProcessor<OS, forceStereo>(2, fpitch, processors, processorConsumesMono, mix,
                                            endpoints, mono34, tempbuf34, tempbuf34);
    }
    if (processors[3])
    {
        muteIn34 |= processors[3]->isMuteProcessor();
        runSingleProcessor<OS, forceStereo>(3, fpitch, processors, processorConsumesMono, mix,
                                            endpoints, mono34, tempbuf34, tempbuf34);
    }

    if constexpr (!forceStereo)
    {
        if (mono12 && mono34)
        {
            mech::accumulate_from_to<scxt::blockSize << OS>(tempbuf12[0], tempbuf34[0]);
            mech::copy_from_to<scxt::blockSize << OS>(tempbuf34[0], output[0]);
        }
        else if (mono12)
        {
            // 34 is stereo so
            mech::accumulate_from_to<scxt::blockSize << OS>(tempbuf12[0], tempbuf34[0]);
            // this 0/1 is right. mono 12 ont stereo 32
            mech::accumulate_from_to<scxt::blockSize << OS>(tempbuf12[0], tempbuf34[1]);
            mech::copy_from_to<scxt::blockSize << OS>(tempbuf34[0], output[0]);
            mech::copy_from_to<scxt::blockSize << OS>(tempbuf34[1], output[1]);
        }
        else if (mono34)
        {
            mech::accumulate_from_to<scxt::blockSize << OS>(tempbuf34[0], tempbuf12[0]);
            // this 0/1 is right. mono 34 ont stereo 12
            mech::accumulate_from_to<scxt::blockSize << OS>(tempbuf34[0], tempbuf12[1]);
            mech::copy_from_to<scxt::blockSize << OS>(tempbuf12[0], output[0]);
            mech::copy_from_to<scxt::blockSize << OS>(tempbuf12[1], output[1]);
        }
        else
        {
            // both sides stereo
            mech::accumulate_from_to<scxt::blockSize << OS>(tempbuf12[0], tempbuf34[0]);
            mech::accumulate_from_to<scxt::blockSize << OS>(tempbuf12[1], tempbuf34[1]);

            mech::copy_from_to<scxt::blockSize << OS>(tempbuf34[0], output[0]);
            mech::copy_from_to<scxt::blockSize << OS>(tempbuf34[1], output[1]);
        }

        chainIsMono = mono12 && mono34;
    }
    else
    {
        mech::accumulate_from_to<scxt::blockSize << OS>(tempbuf12[0], tempbuf34[0]);
        mech::accumulate_from_to<scxt::blockSize << OS>(tempbuf12[1], tempbuf34[1]);

        mech::copy_from_to<scxt::blockSize << OS>(tempbuf34[0], output[0]);
        mech::copy_from_to<scxt::blockSize << OS>(tempbuf34[1], output[1]);
    }

    if (!muteIn12 && !muteIn34)
    {
        // since this sends the signal down both chains, half the
        // amplitude.
        // static constexpr float amp{0.707106781186548f};
        static constexpr float amp{0.5f};
        if (chainIsMono)
        {
            mech::scale_by<scxt::blockSize << OS>(amp, output[0]);
        }
        else
        {
            mech::scale_by<scxt::blockSize << OS>(amp, output[0], output[1]);
        }
    }
}
} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_ROUTING_H
