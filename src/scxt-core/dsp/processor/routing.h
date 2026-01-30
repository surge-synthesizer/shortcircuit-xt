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

#ifndef SCXT_SRC_SCXT_CORE_DSP_PROCESSOR_ROUTING_H
#define SCXT_SRC_SCXT_CORE_DSP_PROCESSOR_ROUTING_H

#include "sst/basic-blocks/mechanics/block-ops.h"

#include "processor.h"

namespace scxt::dsp::processor
{

template <bool OS, bool forceStereo, int N, typename Mix, typename Endpoints>
inline void runSingleProcessor(int i, float fpitch, Processor *processors[engine::processorCount],
                               bool processorConsumesMono[engine::processorCount], Mix &mix,
                               Mix &outLev, Endpoints *endpoints, bool &chainIsMono,
                               float input[2][N], float output[2][N])
{
    if (processors[i]->bypassAnyway)
    {
        return;
    }

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

        chainIsMono = false;
    }
    else
    {
        if (chainIsMono && processorConsumesMono[i] &&
            !processors[i]->monoInputCreatesStereoOutput())
        {
            // mono to mono
            processors[i]->process_monoToMono(input[0], tempbuf[0], fpitch);
            mix[i].fade_blocks(input[0], tempbuf[0], output[0]);
        }
        else if (chainIsMono && processorConsumesMono[i])
        {
            assert(processors[i]->monoInputCreatesStereoOutput());
            // mono to stereo. process then toggle
            processors[i]->process_monoToStereo(input[0], tempbuf[0], tempbuf[1], fpitch);

            // this input[0] is NOT a typo. Input is mono
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
            chainIsMono = false;
        }
    }

    auto ol = *endpoints->processorTarget[i].outputLevelDbP;
    ol = ol * ol * ol * dsp::processor::ProcessorStorage::maxOutputAmp;
    outLev[i].set_target(ol);

    if (forceStereo || !chainIsMono)
    {
        outLev[i].multiply_2_blocks(output[0], output[1]);
    }
    else
    {
        outLev[i].multiply_block(output[0]);
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
                              Mix &outLev, Endpoints *endpoints, bool &chainIsMono,
                              float output[2][N])
{
    namespace mech = sst::basic_blocks::mechanics;

    for (auto i = 0; i < engine::processorCount; ++i)
    {
        if (processors[i])
        {
            runSingleProcessor<OS, forceStereo>(i, fpitch, processors, processorConsumesMono, mix,
                                                outLev, endpoints, chainIsMono, output, output);
        }
    }
}

// -> { A | B } ->
template <bool OS, bool forceStereo, int N, typename Mix, typename Endpoints>
void processParallelPair(int A, int B, float fpitch, Processor *processors[engine::processorCount],
                         bool processorConsumesMono[engine::processorCount], Mix &mix, Mix &outLev,
                         Endpoints *endpoints, bool &chainIsMono, float output[2][N])
{
    namespace mech = sst::basic_blocks::mechanics;

    auto m1 = chainIsMono;
    auto m2 = chainIsMono;

    float tmpbuf alignas(16)[2][2][N];
    bool isMute[2]{true, true};

    if (processors[A] && !processors[A]->bypassAnyway)
    {
        isMute[0] = false;
        runSingleProcessor<OS, forceStereo>(A, fpitch, processors, processorConsumesMono, mix,
                                            outLev, endpoints, m1, output, tmpbuf[0]);
    }

    if (processors[B] && !processors[B]->bypassAnyway)
    {
        isMute[1] = false;
        runSingleProcessor<OS, forceStereo>(B, fpitch, processors, processorConsumesMono, mix,
                                            outLev, endpoints, m2, output, tmpbuf[1]);
    }

    chainIsMono = m1 && m2;

    if (isMute[0] && isMute[1])
    {
        // Both bypassed. Leave output unchanged
        return;
    }

    auto scale = 1.0 / (!isMute[0] + !isMute[1]);
    if (forceStereo || !m1 || !m2)
    {
        // stereo
        mech::clear_block<blockSize << (OS ? 1 : 0)>(output[0]);
        mech::clear_block<blockSize << (OS ? 1 : 0)>(output[1]);

        if (!isMute[0])
        {
            mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[0][0], scale,
                                                                      output[0]);
            mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[0][m1 ? 0 : 1], scale,
                                                                      output[1]);
        }
        if (!isMute[1])
        {
            mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[1][0], scale,
                                                                      output[0]);
            mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[1][m2 ? 0 : 1], scale,
                                                                      output[1]);
        }
    }
    else
    {
        // m1 and m2 are both mono
        mech::clear_block<blockSize << (OS ? 1 : 0)>(output[0]);

        if (!isMute[0])
        {
            mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[0][0], scale,
                                                                      output[0]);
        }
        if (!isMute[1])
        {
            mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[1][0], scale,
                                                                      output[0]);
        }
    }
}

// -> { 1 | 2 } -> { 3 | 4 } ->
template <bool OS, bool forceStereo, int N, typename Mix, typename Endpoints>
void processSer2Pattern(float fpitch, Processor *processors[engine::processorCount],
                        bool processorConsumesMono[engine::processorCount], Mix &mix, Mix &outLev,
                        Endpoints *endpoints, bool &chainIsMono, float output[2][N])
{
    if (processors[0] || processors[1])
    {
        processParallelPair<OS, forceStereo>(0, 1, fpitch, processors, processorConsumesMono, mix,
                                             outLev, endpoints, chainIsMono, output);
    }

    if (processors[2] || processors[3])
    {
        processParallelPair<OS, forceStereo>(2, 3, fpitch, processors, processorConsumesMono, mix,
                                             outLev, endpoints, chainIsMono, output);
    }
}

// -> 1 -> { 2 | 3 } -> 4 ->
template <bool OS, bool forceStereo, int N, typename Mix, typename Endpoints>
void processSer3Pattern(float fpitch, Processor *processors[engine::processorCount],
                        bool processorConsumesMono[engine::processorCount], Mix &mix, Mix &outLev,
                        Endpoints *endpoints, bool &chainIsMono, float output[2][N])
{
    if (processors[0])
    {
        runSingleProcessor<OS, forceStereo>(0, fpitch, processors, processorConsumesMono, mix,
                                            outLev, endpoints, chainIsMono, output, output);
    }

    if (processors[1] || processors[2])
    {
        processParallelPair<OS, forceStereo>(1, 2, fpitch, processors, processorConsumesMono, mix,
                                             outLev, endpoints, chainIsMono, output);
    }

    if (processors[3])
    {
        runSingleProcessor<OS, forceStereo>(3, fpitch, processors, processorConsumesMono, mix,
                                            outLev, endpoints, chainIsMono, output, output);
    }
}

// All in parallel
template <bool OS, bool forceStereo, int N, typename Mix, typename Endpoints>
void processPar1Pattern(float fpitch, Processor *processors[engine::processorCount],
                        bool processorConsumesMono[engine::processorCount], Mix &mix, Mix &outLev,
                        Endpoints *endpoints, bool &chainIsMono, float output[2][N])
{
    namespace mech = sst::basic_blocks::mechanics;

    float tmpbuf alignas(16)[engine::processorCount][2][N];
    bool isMute[engine::processorCount]{};
    static_assert(engine::processorCount == 4); // for the init below
    bool localMono[engine::processorCount]{chainIsMono, chainIsMono, chainIsMono, chainIsMono};

    int numUnMuted = 0;

    for (int i = 0; i < engine::processorCount; ++i)
    {
        if (processors[i])
        {
            isMute[i] = false;
            runSingleProcessor<OS, forceStereo>(i, fpitch, processors, processorConsumesMono, mix,
                                                outLev, endpoints, localMono[i], output, tmpbuf[i]);
        }
        else
        {
            isMute[i] = true;
        }
        numUnMuted += (isMute[i] ? 0 : 1);
    }

    bool isStereo = forceStereo || !(localMono[0] && localMono[1] && localMono[2] && localMono[3]);

    if (numUnMuted == 0)
    {
        return;
    }

    if (isStereo)
    {
        mech::clear_block<blockSize << (OS ? 1 : 0)>(output[0]);
        mech::clear_block<blockSize << (OS ? 1 : 0)>(output[1]);
        float scale = 1.f / numUnMuted;
        for (int i = 0; i < engine::processorCount; ++i)
        {
            if (!isMute[i])
            {
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[i][0], scale,
                                                                          output[0]);
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(
                    tmpbuf[i][localMono[i] ? 0 : 1], scale, output[1]);
            }
        }
        chainIsMono = false;
    }
    else
    {
        mech::clear_block<blockSize << (OS ? 1 : 0)>(output[0]);
        float scale = 1.f / numUnMuted;
        for (int i = 0; i < engine::processorCount; ++i)
        {
            if (!isMute[i])
            {
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[i][0], scale,
                                                                          output[0]);
            }
        }
        chainIsMono = true;
    }
}

// -> { { 1->2} | { 3->4 } ->
template <bool OS, bool forceStereo, int N, typename Mix, typename Endpoints>
void processPar2Pattern(float fpitch, Processor *processors[engine::processorCount],
                        bool processorConsumesMono[engine::processorCount], Mix &mix, Mix &outLev,
                        Endpoints *endpoints, bool &chainIsMono, float output[2][N])
{
    namespace mech = sst::basic_blocks::mechanics;

    float tempbuf12 alignas(16)[2][BLOCK_SIZE << 1];
    float tempbuf34 alignas(16)[2][BLOCK_SIZE << 1];

    bool mono12 = chainIsMono;
    bool mono34 = chainIsMono;

    bool muteIn12 = true;
    bool muteIn34 = true;

    if (processors[0] || processors[1])
    {
        muteIn12 = false;
        mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(output[0], tempbuf12[0]);
        mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(output[1], tempbuf12[1]);
    }
    else
    {
        mech::clear_block<scxt::blockSize << (OS ? 1 : 0)>(tempbuf12[0]);
        mech::clear_block<scxt::blockSize << (OS ? 1 : 0)>(tempbuf12[1]);
    }

    if (processors[0])
    {
        runSingleProcessor<OS, forceStereo>(0, fpitch, processors, processorConsumesMono, mix,
                                            outLev, endpoints, mono12, tempbuf12, tempbuf12);
    }
    if (processors[1])
    {
        runSingleProcessor<OS, forceStereo>(1, fpitch, processors, processorConsumesMono, mix,
                                            outLev, endpoints, mono12, tempbuf12, tempbuf12);
    }

    if (processors[2] || processors[3])
    {
        muteIn34 = false;
        mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(output[0], tempbuf34[0]);
        mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(output[1], tempbuf34[1]);
    }
    else
    {

        mech::clear_block<scxt::blockSize << (OS ? 1 : 0)>(tempbuf34[0]);
        mech::clear_block<scxt::blockSize << (OS ? 1 : 0)>(tempbuf34[1]);
    }

    if (processors[2])
    {
        runSingleProcessor<OS, forceStereo>(2, fpitch, processors, processorConsumesMono, mix,
                                            outLev, endpoints, mono34, tempbuf34, tempbuf34);
    }
    if (processors[3])
    {
        runSingleProcessor<OS, forceStereo>(3, fpitch, processors, processorConsumesMono, mix,
                                            outLev, endpoints, mono34, tempbuf34, tempbuf34);
    }

    if constexpr (!forceStereo)
    {
        if (mono12 && mono34)
        {
            mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf12[0], tempbuf34[0]);
            mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf34[0], output[0]);
        }
        else if (mono12)
        {
            // 34 is stereo so
            mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf12[0], tempbuf34[0]);
            // this 0/1 is right. mono 12 ont stereo 32
            mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf12[0], tempbuf34[1]);
            mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf34[0], output[0]);
            mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf34[1], output[1]);
        }
        else if (mono34)
        {
            mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf34[0], tempbuf12[0]);
            // this 0/1 is right. mono 34 ont stereo 12
            mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf34[0], tempbuf12[1]);
            mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf12[0], output[0]);
            mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf12[1], output[1]);
        }
        else
        {
            // both sides stereo
            mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf12[0], tempbuf34[0]);
            mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf12[1], tempbuf34[1]);

            mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf34[0], output[0]);
            mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf34[1], output[1]);
        }

        chainIsMono = mono12 && mono34;
    }
    else
    {
        mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf12[0], tempbuf34[0]);
        mech::accumulate_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf12[1], tempbuf34[1]);

        mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf34[0], output[0]);
        mech::copy_from_to<scxt::blockSize << (OS ? 1 : 0)>(tempbuf34[1], output[1]);
    }

    if (!muteIn12 && !muteIn34)
    {
        // since this sends the signal down both chains, half the
        // amplitude.
        // static constexpr float amp{0.707106781186548f};
        static constexpr float amp{0.5f};
        if (chainIsMono)
        {
            mech::scale_by<scxt::blockSize << (OS ? 1 : 0)>(amp, output[0]);
        }
        else
        {
            mech::scale_by<scxt::blockSize << (OS ? 1 : 0)>(amp, output[0], output[1]);
        }
    }
}

// -> { 1 | 2 | 3 } -> 4
template <bool OS, bool forceStereo, int N, typename Mix, typename Endpoints>
void processPar3Pattern(float fpitch, Processor *processors[engine::processorCount],
                        bool processorConsumesMono[engine::processorCount], Mix &mix, Mix &outLev,
                        Endpoints *endpoints, bool &chainIsMono, float output[2][N])
{
    namespace mech = sst::basic_blocks::mechanics;

    bool processed = false;

    if (processors[0] || processors[1] || processors[2])
    {
        bool m1 = chainIsMono;
        bool m2 = chainIsMono;
        bool m3 = chainIsMono;

        float tmpbuf alignas(16)[3][2][N];
        bool isMute[3]{true, true, true};

        if (processors[0])
        {
            isMute[0] = false;
            runSingleProcessor<OS, forceStereo>(0, fpitch, processors, processorConsumesMono, mix,
                                                outLev, endpoints, m1, output, tmpbuf[0]);
        }

        if (processors[1])
        {
            isMute[1] = false;
            runSingleProcessor<OS, forceStereo>(1, fpitch, processors, processorConsumesMono, mix,
                                                outLev, endpoints, m2, output, tmpbuf[1]);
        }

        if (processors[2])
        {
            isMute[2] = false;
            runSingleProcessor<OS, forceStereo>(2, fpitch, processors, processorConsumesMono, mix,
                                                outLev, endpoints, m3, output, tmpbuf[2]);
        }

        chainIsMono = m1 && m2 && m3;
        bool isStereo = forceStereo || !chainIsMono;
        assert(!isMute[0] || !isMute[1] || !isMute[2]);
        auto scale = 1.f / (!isMute[0] + !isMute[1] + !isMute[2]);
        if (!isStereo)
        {
            mech::clear_block<blockSize << (OS ? 1 : 0)>(output[0]);
            if (!isMute[0])
            {
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[0][0], scale,
                                                                          output[0]);
            }
            if (!isMute[1])
            {
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[1][0], scale,
                                                                          output[0]);
            }
            if (!isMute[2])
            {
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[2][0], scale,
                                                                          output[0]);
            }
        }
        else
        {
            mech::clear_block<blockSize << (OS ? 1 : 0)>(output[0]);
            mech::clear_block<blockSize << (OS ? 1 : 0)>(output[1]);
            if (!isMute[0])
            {
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[0][0], scale,
                                                                          output[0]);
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[0][m1 ? 0 : 1],
                                                                          scale, output[1]);
            }
            if (!isMute[1])
            {
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[1][0], scale,
                                                                          output[0]);
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[1][m2 ? 0 : 1],
                                                                          scale, output[1]);
            }
            if (!isMute[2])
            {
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[2][0], scale,
                                                                          output[0]);
                mech::scale_accumulate_from_to<blockSize << (OS ? 1 : 0)>(tmpbuf[2][m3 ? 0 : 1],
                                                                          scale, output[1]);
            }
        }
    }

    if (processors[3])
    {
        runSingleProcessor<OS, forceStereo>(3, fpitch, processors, processorConsumesMono, mix,
                                            outLev, endpoints, chainIsMono, output, output);
    }
}

} // namespace scxt::dsp::processor
#endif // SHORTCIRCUITXT_ROUTING_H
