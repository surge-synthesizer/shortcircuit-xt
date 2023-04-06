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

#include "generator.h"
#include "infrastructure/sse_include.h"

#include "resampling.h"
#include "data_tables.h"
#include <fstream>
#include <iostream>
#include <algorithm>

#include "sst/basic-blocks/mechanics/simd-ops.h"
#include "utils.h"
#include <array>
#include <cassert>

namespace scxt::dsp
{
const float I16InvScale = (1.f / (16384.f * 32768.f));
const __m128 I16InvScale_m128 = _mm_set1_ps(I16InvScale);

template <int compoundConfig>
void GeneratorSample(GeneratorState *__restrict GD, GeneratorIO *__restrict IO);

int toLoopValue(bool active, bool forward, bool whileGated, bool isFloat, bool isStereo)
{
    return ((isStereo * 1) << 4) + ((isFloat * 1) << 3) + ((active * 1) << 2) +
           ((forward * 1) << 1) + (whileGated * 1);
}

constexpr std::array<bool, 5> fromLoopValue(int lv)
{
    bool whileGated = (lv & (1 << 0));
    bool forward = (lv & (1 << 1));
    bool active = (lv & (1 << 2));
    bool isfl = (lv & (1 << 3));
    bool stereo = (lv & (1 << 4));
    return {active, forward, whileGated, isfl, stereo};
}

namespace detail
{
using genOp_t = GeneratorFPtr (*)();
template <size_t I> GeneratorFPtr implGeneratorGetImpl() { return GeneratorSample<I>; }

template <size_t... Is> auto generatorGet(size_t ft, std::index_sequence<Is...>)
{
    constexpr genOp_t fnc[] = {detail::implGeneratorGetImpl<Is>...};
    return fnc[ft]();
}
} // namespace detail

GeneratorFPtr GetFPtrGeneratorSample(bool Stereo, bool Float, bool loopActive, bool loopForward,
                                     bool loopWhileGated)
{
    auto loopValue = toLoopValue(loopActive, loopForward, loopWhileGated, Float, Stereo);
    assert(loopValue >= 0 && loopValue < (1 << 5));
    return detail::generatorGet(loopValue, std::make_index_sequence<(1 << 5) - 1>());
}

template <int loopValue>
void GeneratorSample(GeneratorState *__restrict GD, GeneratorIO *__restrict IO)
{
    static constexpr auto mode = fromLoopValue(loopValue);
    static constexpr auto loopActive = std::get<0>(mode);
    static constexpr auto loopForward = std::get<1>(mode);
    static constexpr auto loopWhileGated = std::get<2>(mode);
    static constexpr auto fp = std::get<3>(mode);
    static constexpr auto stereo = std::get<4>(mode);

    int SamplePos = GD->samplePos;
    int SampleSubPos = GD->sampleSubPos;
    int IsFinished = GD->isFinished;
    int WaveSize = IO->waveSize;
    int LoopOffset = std::max(1, GD->loopUpperBound - GD->loopLowerBound);
    int Ratio = GD->ratio;
    int RatioSign = Ratio < 0 ? -1 : 1;
    Ratio = std::abs(Ratio);
    int Direction = GD->direction * RatioSign;
    int16_t *__restrict SampleDataL;
    int16_t *__restrict SampleDataR;
    float *__restrict SampleDataFL;
    float *__restrict SampleDataFR;
    float *__restrict OutputL;
    float *__restrict OutputR;

    GD->positionWithinLoop = 0.f;
    GD->isInLoop = false;

    if (fp)
        SampleDataFL = (float *)IO->sampleDataL;
    else
        SampleDataL = (short *)IO->sampleDataL;
    OutputL = IO->outputL;
    if (stereo)
    {
        if (fp)
            SampleDataFR = (float *)IO->sampleDataR;
        SampleDataR = (short *)IO->sampleDataR;
        OutputR = IO->outputR;
    }

    static constexpr int resampFIRSize{16};
    int16_t *__restrict readSampleL = nullptr;
    int16_t *__restrict readSampleR = nullptr;
    int16_t loopEndBufferL[resampFIRSize], loopEndBufferR[resampFIRSize];
    float *__restrict readSampleLF32 = nullptr;
    float *__restrict readSampleRF32 = nullptr;
    float loopEndBufferLF32[resampFIRSize], loopEndBufferRF32[resampFIRSize];

    if (fp)
    {
        readSampleLF32 = SampleDataFL + SamplePos;
        if (stereo)
            readSampleRF32 = SampleDataFR + SamplePos;

        if constexpr (loopActive)
        {
            if (SamplePos >= WaveSize - resampFIRSize && SamplePos <= GD->loopUpperBound)
            {
                for (int k = 0; k < resampFIRSize; ++k)
                {
                    auto q = k + SamplePos;
                    if (q >= GD->loopUpperBound || q >= WaveSize)
                        q -= LoopOffset;
                    loopEndBufferLF32[k] = SampleDataFL[q];
                    if (stereo)
                        loopEndBufferRF32[k] = SampleDataFR[q];
                }
                readSampleLF32 = loopEndBufferLF32;
                if (stereo)
                    readSampleRF32 = loopEndBufferRF32;
            }
        }
    }
    else
    {
        readSampleL = SampleDataL + SamplePos;
        if (stereo)
            readSampleR = SampleDataR + SamplePos;

        if constexpr (loopActive)
        {
            if (SamplePos >= WaveSize - resampFIRSize && SamplePos <= GD->loopUpperBound)
            {
                for (int k = 0; k < resampFIRSize; ++k)
                {
                    auto q = k + SamplePos;
                    if (q >= GD->loopUpperBound || q >= WaveSize)
                        q -= LoopOffset;
                    loopEndBufferL[k] = SampleDataL[q];
                    if (stereo)
                        loopEndBufferR[k] = SampleDataR[q];
                }
                readSampleL = loopEndBufferL;
                if (stereo)
                    readSampleR = loopEndBufferR;
            }
        }
    }

    int NSamples = GD->blockSize;

    int i{0};
    for (i = 0; i < NSamples && !IsFinished; i++)
    {
        // 2. Resample
        unsigned int m0 = ((SampleSubPos >> 12) & 0xff0);
        if (fp)
        {
            // float32 path (SSE)
            __m128 lipol0, tmp[4], sL4, sR4;
            lipol0 = _mm_setzero_ps();
            lipol0 = _mm_cvtsi32_ss(lipol0, SampleSubPos & 0xffff);
            lipol0 = _mm_shuffle_ps(lipol0, lipol0, _MM_SHUFFLE(0, 0, 0, 0));
            tmp[0] = _mm_add_ps(_mm_mul_ps(*((__m128 *)&sincTable.SincOffsetF32[m0]), lipol0),
                                *((__m128 *)&sincTable.SincTableF32[m0]));
            tmp[1] = _mm_add_ps(_mm_mul_ps(*((__m128 *)&sincTable.SincOffsetF32[m0 + 4]), lipol0),
                                *((__m128 *)&sincTable.SincTableF32[m0 + 4]));
            tmp[2] = _mm_add_ps(_mm_mul_ps(*((__m128 *)&sincTable.SincOffsetF32[m0 + 8]), lipol0),
                                *((__m128 *)&sincTable.SincTableF32[m0 + 8]));
            tmp[3] = _mm_add_ps(_mm_mul_ps(*((__m128 *)&sincTable.SincOffsetF32[m0 + 12]), lipol0),
                                *((__m128 *)&sincTable.SincTableF32[m0 + 12]));
            sL4 = _mm_mul_ps(tmp[0], _mm_loadu_ps(readSampleLF32));
            sL4 = _mm_add_ps(sL4, _mm_mul_ps(tmp[1], _mm_loadu_ps(readSampleLF32 + 4)));
            sL4 = _mm_add_ps(sL4, _mm_mul_ps(tmp[2], _mm_loadu_ps(readSampleLF32 + 8)));
            sL4 = _mm_add_ps(sL4, _mm_mul_ps(tmp[3], _mm_loadu_ps(readSampleLF32 + 12)));
            // sL4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sL4);
            sL4 = _mm_hadd_ps(sL4, sL4);
            sL4 = _mm_hadd_ps(sL4, sL4);

            _mm_store_ss(&OutputL[i], sL4);
            if (stereo)
            {
                sR4 = _mm_mul_ps(tmp[0], _mm_loadu_ps(readSampleRF32));
                sR4 = _mm_add_ps(sR4, _mm_mul_ps(tmp[1], _mm_loadu_ps(readSampleRF32 + 4)));
                sR4 = _mm_add_ps(sR4, _mm_mul_ps(tmp[2], _mm_loadu_ps(readSampleRF32 + 8)));
                sR4 = _mm_add_ps(sR4, _mm_mul_ps(tmp[3], _mm_loadu_ps(readSampleRF32 + 12)));
                // sR4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sR4);
                sR4 = _mm_hadd_ps(sR4, sR4);
                sR4 = _mm_hadd_ps(sR4, sR4);

                _mm_store_ss(&OutputR[i], sR4);
            }
        }
        else
        {
            // int16
            // SSE2 path
            __m128i lipol0, tmp, sL8A, sR8A, tmp2, sL8B, sR8B;
            __m128 fL, fR;
            lipol0 = _mm_set1_epi16(SampleSubPos & 0xffff);

            tmp = _mm_add_epi16(_mm_mulhi_epi16(*((__m128i *)&sincTable.SincOffsetI16[m0]), lipol0),
                                *((__m128i *)&sincTable.SincTableI16[m0]));
            sL8A = _mm_madd_epi16(tmp, _mm_loadu_si128((__m128i *)readSampleL));
            if (stereo)
                sR8A = _mm_madd_epi16(tmp, _mm_loadu_si128((__m128i *)readSampleR));
            tmp2 = _mm_add_epi16(
                _mm_mulhi_epi16(*((__m128i *)&sincTable.SincOffsetI16[m0 + 8]), lipol0),
                *((__m128i *)&sincTable.SincTableI16[m0 + 8]));
            sL8B = _mm_madd_epi16(tmp2, _mm_loadu_si128((__m128i *)(readSampleL + 8)));
            if (stereo)
                sR8B = _mm_madd_epi16(tmp2, _mm_loadu_si128((__m128i *)(readSampleR + 8)));
            sL8A = _mm_add_epi32(sL8A, sL8B);
            if (stereo)
                sR8A = _mm_add_epi32(sR8A, sR8B);

            int l alignas(16)[4], r alignas(16)[4];
            _mm_store_si128((__m128i *)&l, sL8A);
            if (stereo)
                _mm_store_si128((__m128i *)&r, sR8A);
            l[0] = (l[0] + l[1]) + (l[2] + l[3]);
            if (stereo)
                r[0] = (r[0] + r[1]) + (r[2] + r[3]);
            fL = _mm_mul_ss(_mm_cvtsi32_ss(fL, l[0]), I16InvScale_m128);
            if (stereo)
                fR = _mm_mul_ss(_mm_cvtsi32_ss(fR, r[0]), I16InvScale_m128);
            _mm_store_ss(&OutputL[i], fL);
            if (stereo)
                _mm_store_ss(&OutputR[i], fR);
        }

        // 3. Forward sample position
        SampleSubPos += Ratio * Direction;
        int incr = SampleSubPos >> 24;
        SamplePos += incr;
        SampleSubPos = SampleSubPos - (incr << 24);

        if constexpr (!loopActive) // these constexprs just remind us not to refactor to break ce
        {
            // No loop, simple case: Play the bounds then done.
            if (SamplePos > GD->playbackUpperBound)
            {
                SamplePos = GD->playbackUpperBound;
                SampleSubPos = 0;
                if (GD->direction == 1)
                    IsFinished = true;
            }
            if (SamplePos < GD->playbackLowerBound)
            {
                SamplePos = GD->playbackLowerBound;
                SampleSubPos = 0;
                if (GD->direction == -1)
                    IsFinished = true;
            }

            if constexpr (fp)
            {
                readSampleLF32 = SampleDataFL + SamplePos;
                if (stereo)
                    readSampleRF32 = SampleDataFR + SamplePos;
            }
            else
            {
                readSampleL = SampleDataL + SamplePos;
                if (stereo)
                    readSampleR = SampleDataR + SamplePos;
            }
        }
        else if constexpr (!loopWhileGated && loopForward)
        {
            int offset = SamplePos;

            if (Direction > 0)
            {
                // Upper
                if (offset > GD->loopUpperBound)
                    offset -= LoopOffset;
            }
            else
            {
                // Lower
                if (offset < GD->loopLowerBound)
                    offset += LoopOffset;
            }

            if (offset > WaveSize || offset < 0)
                offset = GD->loopUpperBound;

            SamplePos = offset;
        }
        else if constexpr (!loopWhileGated && !loopForward)
        {
            // bidirectional
            if (SamplePos >= GD->loopUpperBound)
            {
                Direction = -1;
            }
            else if (SamplePos <= GD->loopLowerBound)
            {
                Direction = 1;
            }

            SamplePos = std::clamp(SamplePos, 0, WaveSize);
        }
        else if constexpr (loopForward)
        {
            // gated forward
            if (GD->gated)
            {
                int offset = SamplePos;

                if (Direction > 0)
                {
                    if (offset > GD->loopUpperBound)
                        offset -= LoopOffset;
                }
                else
                {
                    if (offset < GD->loopLowerBound)
                        offset += LoopOffset;
                }

                if (offset > WaveSize || offset < 0)
                    offset = GD->loopUpperBound;

                SamplePos = offset;
            }
            else
            {
                if (SamplePos > GD->playbackUpperBound)
                {
                    SamplePos = GD->playbackUpperBound;
                    SampleSubPos = 0;
                    if (GD->direction == 1)
                        IsFinished = true;
                }
                if (SamplePos < GD->playbackLowerBound)
                {
                    SamplePos = GD->playbackLowerBound;
                    SampleSubPos = 0;
                    if (GD->direction == -1)
                        IsFinished = true;
                }
            }
        }
        else if constexpr (!loopForward && loopWhileGated)
        {
            // gated bidirecational
            if (GD->gated || (GD->direction != GD->directionAtOutset))
            {
                if (SamplePos >= GD->loopUpperBound)
                {
                    Direction = -1;
                }
                else if (SamplePos <= GD->loopLowerBound)
                {
                    Direction = 1;
                }

                SamplePos = std::clamp(SamplePos, 0, WaveSize);
            }
            else
            {
                // TODO : Careful with releasing a loop while going backwards
                if (SamplePos > GD->playbackUpperBound)
                {
                    SamplePos = GD->playbackUpperBound;
                    SampleSubPos = 0;
                    if (GD->direction == 1)
                        IsFinished = true;
                }
                if (SamplePos < GD->playbackLowerBound)
                {
                    SamplePos = GD->playbackLowerBound;
                    SampleSubPos = 0;
                    if (GD->direction == -1)
                        IsFinished = true;
                }
            }
        }

        if constexpr (loopActive)
        {
            if constexpr (fp)
            {
                if (SamplePos >= WaveSize - resampFIRSize && SamplePos <= GD->loopUpperBound)
                {
                    for (int k = 0; k < resampFIRSize; ++k)
                    {
                        auto q = k + SamplePos;
                        if (q >= GD->loopUpperBound || q >= WaveSize)
                            q -= LoopOffset;
                        loopEndBufferLF32[k] = SampleDataFL[q];
                        if (stereo)
                            loopEndBufferRF32[k] = SampleDataFR[q];
                    }
                    readSampleLF32 = loopEndBufferLF32;
                    if (stereo)
                        readSampleRF32 = loopEndBufferRF32;
                }
                else
                {
                    readSampleLF32 = SampleDataFL + SamplePos;
                    if (stereo)
                        readSampleRF32 = SampleDataFR + SamplePos;
                }
            }
            else
            {
                // we need both checks because if we are just doing a post-release playdown
                // we don't want to re-pad
                if (SamplePos >= WaveSize - resampFIRSize && SamplePos <= GD->loopUpperBound)
                {
                    for (int k = 0; k < resampFIRSize; ++k)
                    {
                        auto q = k + SamplePos;
                        if (q >= GD->loopUpperBound || q >= WaveSize)
                            q -= LoopOffset;
                        loopEndBufferL[k] = SampleDataL[q];
                        if (stereo)
                            loopEndBufferR[k] = SampleDataR[q];
                    }
                    readSampleL = loopEndBufferL;
                    if (stereo)
                        readSampleR = loopEndBufferR;
                }
                else
                {
                    readSampleL = SampleDataL + SamplePos;
                    if (stereo)
                        readSampleR = SampleDataR + SamplePos;
                }
            }
        }
    }

    // Clean up any items left
    for (; i < NSamples; ++i)
    {
        OutputL[i] = 0.f;
        if (stereo)
            OutputR[i] = 0.f;
    }

    GD->direction = Direction * RatioSign;
    GD->samplePos = SamplePos;
    GD->sampleSubPos = SampleSubPos;
    GD->isFinished = IsFinished;

    if constexpr (loopActive)
    {
        if (!loopWhileGated)
        {
            GD->isInLoop = (SamplePos >= GD->loopLowerBound);
            GD->positionWithinLoop =
                std::clamp((SamplePos - GD->loopLowerBound) * GD->loopInvertedBounds, 0.f, 1.f);
        }
        else
        {
            if (GD->gated || (GD->directionAtOutset != GD->direction))
            {
                GD->isInLoop = (SamplePos >= GD->loopLowerBound);
                GD->positionWithinLoop =
                    std::clamp((SamplePos - GD->loopLowerBound) * GD->loopInvertedBounds, 0.f, 1.f);
            }
            else
            {
                GD->isInLoop = false;
                GD->positionWithinLoop =
                    std::clamp((SamplePos - GD->loopLowerBound) * GD->loopInvertedBounds, 0.f, 1.f);
            }
        }
    }
}
} // namespace scxt::dsp