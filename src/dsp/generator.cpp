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

#include "generator.h"

#include "sst/basic-blocks/simd/setup.h"

#include "resampling.h"
#include "data_tables.h"
#include <fstream>
#include <iostream>
#include <algorithm>

#include "sst/basic-blocks/mechanics/simd-ops.h"
#include "utils.h"
#include <array>
#include <cassert>

/*
 * This is the Generator, the core class which moves from the sample data to an output
 * stream. It handles looping, fades, interpolation methods, f32 vs i16 and more.
 *
 * There's three "big ideas" you need to udnerstand it
 *
 * The first is we have multiple processors by template type. Sinc, ZOH, Linear, etc...
 * These have the job that given a block of input and a sample position, generate
 * the appropriate output.
 *
 * The second is we have the generator state which contains sample position, loop information
 * etc... The job of this generator is to advance that sample state to make sure we loop
 *
 * And the last is to deal positionally correctly with the input data mapping the sample
 * pos in the second to the interpolation model in the first. So what do we expect for input
 * sample data?
 *
 * So at input, we expect the sample data in GDIO to be pointing at the top of the sample
 * but that sample is sitting in a buffer which is allocated FIROffset (=8) buffers
 * wider on either side with 0s. That is, we hace valid memory from
 * sample-8 to sample + samplesize + 8 - 1 with zero pads.
 *
 * 0 0 0 0 0 0 0 0 s s s s .... s s s s 0 0 0 0 0 0 0 0
 *                 ^                  ^
 *               sample            sample + waveSize
 *
 * This allows the processors to have a FIRipol_N{16} wide window in which to interpolate
 *
 * So then we get to the last bit of index shuffling. This generator is factored
 * so individual prcoessors have their own process method which generates a samlpe
 * intput output buffer {i} from a given chunk of data. The sample alignment for
 * those processors is such that they get FIRipon_N{16} samples centered around
 * SamplePos with SampleSubPos. Or the expectation is
 *
 * 1 2 3 4 5 6 7 8 9 A B C D E F
 *             | |
 *          SamplePos in this range
 *
 * The job of the harness which calls the processor (GeneratorSample) is to set up
 * that indexing and the job of the processor itself is to find the value. The sinc
 * processor, for instance, uses a 16 wide FIR but the ZOH just uses position FIRoffset-1.
 *
 * Since zero-pad-at-end is not always the right answer (for instance, with a
 * loop you want to pad with the start of the loop) the GeneratorSampel below does the
 * index shuffling and sometimes makes temporaries before calling a processor.
 *
 * The processors also have the opportunity to have a fade region where they
 * cross fade between two sets of FIR-wide sample inputs aligned in the same
 * fashion using the fade value defined below.
 *
 * And everything is templated so we can constexpr out the code we don't need
 * and use a function pointer at voice on with the appropriate compile time config.
 *
 * Good luck!
 */

namespace scxt::dsp
{
constexpr float I16InvScale = (1.f / (16384.f * 32768.f));
constexpr float I16InvScale2 = (1.f / (32768.f));
const auto I16InvScale_m128 = SIMD_MM(set1_ps)(I16InvScale);

inline float getFadeGainToAmp(float g)
{
    // return std::cbrt(g);
    // return 4.f / 3.f * (1 - 1 / ((1 + g) * (1 + g)));
    return 2 * (1 - 1 / (1 + g));
}
inline float getFadeGain(int32_t samplePos, int32_t x1, int32_t x2)
{
    assert(x1 <= samplePos && samplePos <= x2);
    auto gain = ((float)(x1 - samplePos)) / (x1 - x2);
    return gain;
}

template <InterpolationTypes KT, typename T> struct KernelOp
{
};

template <InterpolationTypes KT, typename T, int NUM_CHANNELS, bool LOOP_ACTIVE>
struct KernelProcessor;

template <typename T> struct KernelOp<InterpolationTypes::ZeroOrderHold, T>
{
    template <int NUM_CHANNELS, bool LOOP_ACTIVE>
    static void
    Process(GeneratorState *__restrict GD,
            KernelProcessor<InterpolationTypes::ZeroOrderHold, T, NUM_CHANNELS, LOOP_ACTIVE> &ks);
};

template <> struct KernelOp<InterpolationTypes::ZeroOrderHoldAA, float>
{
    template <int NUM_CHANNELS, bool LOOP_ACTIVE>
    static void Process(
        GeneratorState *__restrict GD,
        KernelProcessor<InterpolationTypes::ZeroOrderHoldAA, float, NUM_CHANNELS, LOOP_ACTIVE> &ks);
};

template <> struct KernelOp<InterpolationTypes::ZeroOrderHoldAA, int16_t>
{
    template <int NUM_CHANNELS, bool LOOP_ACTIVE>
    static void Process(GeneratorState *__restrict GD,
                        KernelProcessor<InterpolationTypes::ZeroOrderHoldAA, int16_t, NUM_CHANNELS,
                                        LOOP_ACTIVE> &ks);
};

template <typename T> struct KernelOp<InterpolationTypes::Linear, T>
{
    template <int NUM_CHANNELS, bool LOOP_ACTIVE>
    static void
    Process(GeneratorState *__restrict GD,
            KernelProcessor<InterpolationTypes::Linear, T, NUM_CHANNELS, LOOP_ACTIVE> &ks);
};

template <> struct KernelOp<InterpolationTypes::Sinc, float>
{
    template <int NUM_CHANNELS, bool LOOP_ACTIVE>
    static void
    Process(GeneratorState *__restrict GD,
            KernelProcessor<InterpolationTypes::Sinc, float, NUM_CHANNELS, LOOP_ACTIVE> &ks);
};

template <> struct KernelOp<InterpolationTypes::Sinc, int16_t>
{
    template <int NUM_CHANNELS, bool LOOP_ACTIVE>
    static void
    Process(GeneratorState *__restrict GD,
            KernelProcessor<InterpolationTypes::Sinc, int16_t, NUM_CHANNELS, LOOP_ACTIVE> &ks);
};

template <InterpolationTypes KT, typename T, int NUM_CHANNELS, bool LOOP_ACTIVE>
struct KernelProcessor
{
    int32_t SamplePos, SampleSubPos;
    int32_t m0, i;

    T *ReadSample[NUM_CHANNELS];

    T *ReadFadeSample[NUM_CHANNELS];
    bool fadeActive;
    int32_t loopFade;

    float *Output[NUM_CHANNELS];

    GeneratorIO *IO{nullptr}; // only used for debug constraints

    void ProcessKernel(GeneratorState *__restrict GD)
    {
        return KernelOp<KT, T>::Process(GD, *this);
    }
};

float NormalizeSampleToF32(float val) { return val; }

float NormalizeSampleToF32(int16_t val) { return val * I16InvScale2; }

template <typename T>
template <int NUM_CHANNELS, bool LOOP_ACTIVE>
void KernelOp<InterpolationTypes::ZeroOrderHold, T>::Process(
    GeneratorState *__restrict GD,
    KernelProcessor<InterpolationTypes::ZeroOrderHold, T, NUM_CHANNELS, LOOP_ACTIVE> &ks)
{
    auto readSampleL{ks.ReadSample[0]};
    auto readFadeSampleL{ks.ReadFadeSample[0]};
    auto OutputL{ks.Output[0]};
    auto i{ks.i};

    auto readPos = FIRoffset - 1;
    OutputL[i] = NormalizeSampleToF32(readSampleL[readPos]);

    if constexpr (LOOP_ACTIVE)
    {
        if (ks.fadeActive)
        {
            auto fadeVal{NormalizeSampleToF32(readFadeSampleL[readPos])};
            auto fadeGain(
                getFadeGain(ks.SamplePos, GD->loopUpperBound - ks.loopFade, GD->loopUpperBound));
            auto aOut = getFadeGainToAmp(1.f - fadeGain);
            fadeGain = getFadeGainToAmp(fadeGain);

            OutputL[i] = OutputL[i] * aOut + fadeVal * fadeGain;
        }
    }

    if constexpr (NUM_CHANNELS == 2)
    {
        auto readSampleR{ks.ReadSample[1]};
        auto readFadeSampleR{ks.ReadFadeSample[1]};
        auto OutputR{ks.Output[1]};

        OutputR[i] = NormalizeSampleToF32(readSampleR[readPos]);

        if constexpr (LOOP_ACTIVE)
        {
            if (ks.fadeActive)
            {
                float fadeVal{NormalizeSampleToF32(readFadeSampleR[readPos])};
                auto fadeGain(getFadeGain(ks.SamplePos, GD->loopUpperBound - ks.loopFade,
                                          GD->loopUpperBound));
                auto aOut = getFadeGainToAmp(1.f - fadeGain);
                fadeGain = getFadeGainToAmp(fadeGain);

                OutputR[i] = OutputR[i] * aOut + fadeVal * fadeGain;
            }
        }
    }
}

template <int NUM_CHANNELS, bool LOOP_ACTIVE>
void KernelOp<InterpolationTypes::ZeroOrderHoldAA, float>::Process(
    GeneratorState *__restrict GD,
    KernelProcessor<InterpolationTypes::ZeroOrderHoldAA, float, NUM_CHANNELS, LOOP_ACTIVE> &ks)
{
    auto readSampleL{ks.ReadSample[0]};
    auto readFadeSampleL{ks.ReadFadeSample[0]};
    auto OutputL{ks.Output[0]};
    auto m0{ks.m0};
    auto i{ks.i};

#if DEBUG_CODE_I_WANT_TO_RETAIN
    /* This code only works if theres no loop etc but please leave it here
     * since it is useful when debugging in the future
     */
    {
        ptrdiff_t space = (float *)readSampleL - (float *)ks.IO->sampleDataL;
        float above = space - ks.IO->waveSize + FIRipol_N;

        if (space < -(int64_t)FIRoffset || above > FIRoffset)
            SCLOG(SCD(ks.IO->sampleDataL)
                  << SCD(readSampleL) << SCD(space) << SCD(above) << SCD(ks.IO->waveSize));

        assert(space >= -(int64_t)FIRoffset && above <= FIRoffset);
    }
#endif

    auto finalSubPos = ks.SampleSubPos;
    auto subSubPos = (float)(finalSubPos) / (float)(1 << 24);
    auto subRatio = std::abs((float)(GD->ratio) / (float)(1 << 24));
    if (subRatio <= 0.5f)
    {
        subSubPos = std::pow(subSubPos, 1.0f / subRatio);
        finalSubPos = (int)(subSubPos * (1 << 24));
        m0 = ((finalSubPos >> 12) & 0xff0);
    }

    // float32 path (SSE)
    SIMD_M128 lipol0, tmp[4], sL4, sR4;
    lipol0 = SIMD_MM(setzero_ps)();
    lipol0 = SIMD_MM(cvtsi32_ss)(lipol0, finalSubPos & 0xffff);
    lipol0 = SIMD_MM(shuffle_ps)(lipol0, lipol0, SIMD_MM_SHUFFLE(0, 0, 0, 0));
    tmp[0] = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(*((SIMD_M128 *)&sincTable.SincOffsetF32[m0]), lipol0),
                             *((SIMD_M128 *)&sincTable.SincTableF32[m0]));
    tmp[1] =
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(*((SIMD_M128 *)&sincTable.SincOffsetF32[m0 + 4]), lipol0),
                        *((SIMD_M128 *)&sincTable.SincTableF32[m0 + 4]));
    tmp[2] =
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(*((SIMD_M128 *)&sincTable.SincOffsetF32[m0 + 8]), lipol0),
                        *((SIMD_M128 *)&sincTable.SincTableF32[m0 + 8]));
    tmp[3] =
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(*((SIMD_M128 *)&sincTable.SincOffsetF32[m0 + 12]), lipol0),
                        *((SIMD_M128 *)&sincTable.SincTableF32[m0 + 12]));
    sL4 = SIMD_MM(mul_ps)(tmp[0], SIMD_MM(loadu_ps)(readSampleL));
    sL4 = SIMD_MM(add_ps)(sL4, SIMD_MM(mul_ps)(tmp[1], SIMD_MM(loadu_ps)(readSampleL + 4)));
    sL4 = SIMD_MM(add_ps)(sL4, SIMD_MM(mul_ps)(tmp[2], SIMD_MM(loadu_ps)(readSampleL + 8)));
    sL4 = SIMD_MM(add_ps)(sL4, SIMD_MM(mul_ps)(tmp[3], SIMD_MM(loadu_ps)(readSampleL + 12)));
    // sL4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sL4);
    sL4 = SIMD_MM(hadd_ps)(sL4, sL4);
    sL4 = SIMD_MM(hadd_ps)(sL4, sL4);

    SIMD_MM(store_ss)(&OutputL[i], sL4);

    if constexpr (LOOP_ACTIVE)
    {
        if (ks.fadeActive)
        {
            sR4 = SIMD_MM(mul_ps)(tmp[0], SIMD_MM(loadu_ps)(readFadeSampleL));
            sR4 = SIMD_MM(add_ps)(sR4,
                                  SIMD_MM(mul_ps)(tmp[1], SIMD_MM(loadu_ps)(readFadeSampleL + 4)));
            sR4 = SIMD_MM(add_ps)(sR4,
                                  SIMD_MM(mul_ps)(tmp[2], SIMD_MM(loadu_ps)(readFadeSampleL + 8)));
            sR4 = SIMD_MM(add_ps)(sR4,
                                  SIMD_MM(mul_ps)(tmp[3], SIMD_MM(loadu_ps)(readFadeSampleL + 12)));
            // sR4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sR4);
            sR4 = SIMD_MM(hadd_ps)(sR4, sR4);
            sR4 = SIMD_MM(hadd_ps)(sR4, sR4);

            float fadeVal{0.f};
            SIMD_MM(store_ss)(&fadeVal, sR4);
            auto fadeGain(
                getFadeGain(ks.SamplePos, GD->loopUpperBound - ks.loopFade, GD->loopUpperBound));
            auto aOut = getFadeGainToAmp(1.f - fadeGain);
            fadeGain = getFadeGainToAmp(fadeGain);

            OutputL[i] = OutputL[i] * aOut + fadeVal * fadeGain;
        }
    }

    if constexpr (NUM_CHANNELS == 2)
    {
        auto readSampleR{ks.ReadSample[1]};
        auto readFadeSampleR{ks.ReadFadeSample[1]};
        auto OutputR{ks.Output[1]};

        sR4 = SIMD_MM(mul_ps)(tmp[0], SIMD_MM(loadu_ps)(readSampleR));
        sR4 = SIMD_MM(add_ps)(sR4, SIMD_MM(mul_ps)(tmp[1], SIMD_MM(loadu_ps)(readSampleR + 4)));
        sR4 = SIMD_MM(add_ps)(sR4, SIMD_MM(mul_ps)(tmp[2], SIMD_MM(loadu_ps)(readSampleR + 8)));
        sR4 = SIMD_MM(add_ps)(sR4, SIMD_MM(mul_ps)(tmp[3], SIMD_MM(loadu_ps)(readSampleR + 12)));
        // sR4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sR4);
        sR4 = SIMD_MM(hadd_ps)(sR4, sR4);
        sR4 = SIMD_MM(hadd_ps)(sR4, sR4);

        SIMD_MM(store_ss)(&OutputR[i], sR4);

        if constexpr (LOOP_ACTIVE)
        {
            if (ks.fadeActive)
            {
                sR4 = SIMD_MM(mul_ps)(tmp[0], SIMD_MM(loadu_ps)(readFadeSampleR));
                sR4 = SIMD_MM(add_ps)(
                    sR4, SIMD_MM(mul_ps)(tmp[1], SIMD_MM(loadu_ps)(readFadeSampleR + 4)));
                sR4 = SIMD_MM(add_ps)(
                    sR4, SIMD_MM(mul_ps)(tmp[2], SIMD_MM(loadu_ps)(readFadeSampleR + 8)));
                sR4 = SIMD_MM(add_ps)(
                    sR4, SIMD_MM(mul_ps)(tmp[3], SIMD_MM(loadu_ps)(readFadeSampleR + 12)));
                // sR4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sR4);
                sR4 = SIMD_MM(hadd_ps)(sR4, sR4);
                sR4 = SIMD_MM(hadd_ps)(sR4, sR4);

                float fadeVal{0.f};
                SIMD_MM(store_ss)(&fadeVal, sR4);
                auto fadeGain(getFadeGain(ks.SamplePos, GD->loopUpperBound - ks.loopFade,
                                          GD->loopUpperBound));
                auto aOut = getFadeGainToAmp(1.f - fadeGain);
                fadeGain = getFadeGainToAmp(fadeGain);

                OutputR[i] = OutputR[i] * aOut + fadeVal * fadeGain;
            }
        }
    }
}

template <int NUM_CHANNELS, bool LOOP_ACTIVE>
void KernelOp<InterpolationTypes::ZeroOrderHoldAA, int16_t>::Process(
    GeneratorState *__restrict GD,
    KernelProcessor<InterpolationTypes::ZeroOrderHoldAA, int16_t, NUM_CHANNELS, LOOP_ACTIVE> &ks)
{
    auto readSampleL{ks.ReadSample[0]};
    auto readFadeSampleL{ks.ReadFadeSample[0]};
    auto OutputL{ks.Output[0]};
    auto m0{ks.m0};
    auto i{ks.i};
    constexpr auto stereo = NUM_CHANNELS == 2;

    int16_t *readSampleR{nullptr};
    float *OutputR{nullptr};
    if constexpr (stereo)
    {
        readSampleR = ks.ReadSample[1];
        OutputR = ks.Output[1];
    }

    auto finalSubPos = ks.SampleSubPos;
    auto subSubPos = (float)(finalSubPos) / (float)(1 << 24);
    auto subRatio = std::abs((float)(GD->ratio) / (float)(1 << 24));
    if (subRatio <= 0.5f)
    {
        subSubPos = std::pow(subSubPos, 1.0f / subRatio);
        finalSubPos = (int)(subSubPos * (1 << 24));
        m0 = ((finalSubPos >> 12) & 0xff0);
    }

    // int16
    // SSE2 path
    SIMD_M128I lipol0, tmp, sL8A, sR8A, tmp2, sL8B, sR8B;
    auto fL = SIMD_MM(setzero_ps)(), fR = SIMD_MM(setzero_ps)();
    lipol0 = SIMD_MM(set1_epi16)(finalSubPos & 0xffff);

    tmp = SIMD_MM(add_epi16)(
        SIMD_MM(mulhi_epi16)(*((SIMD_M128I *)&sincTable.SincOffsetI16[m0]), lipol0),
        *((SIMD_M128I *)&sincTable.SincTableI16[m0]));
    sL8A = SIMD_MM(madd_epi16)(tmp, SIMD_MM(loadu_si128)((SIMD_M128I *)readSampleL));
    if constexpr (stereo)
        sR8A = SIMD_MM(madd_epi16)(tmp, SIMD_MM(loadu_si128)((SIMD_M128I *)readSampleR));

    tmp2 = SIMD_MM(add_epi16)(
        SIMD_MM(mulhi_epi16)(*((SIMD_M128I *)&sincTable.SincOffsetI16[m0 + 8]), lipol0),
        *((SIMD_M128I *)&sincTable.SincTableI16[m0 + 8]));
    sL8B = SIMD_MM(madd_epi16)(tmp2, SIMD_MM(loadu_si128)((SIMD_M128I *)(readSampleL + 8)));
    if constexpr (stereo)
        sR8B = SIMD_MM(madd_epi16)(tmp2, SIMD_MM(loadu_si128)((SIMD_M128I *)(readSampleR + 8)));

    sL8A = SIMD_MM(add_epi32)(sL8A, sL8B);
    if constexpr (stereo)
        sR8A = SIMD_MM(add_epi32)(sR8A, sR8B);

    int l alignas(16)[4], r alignas(16)[4];
    SIMD_MM(store_si128)((SIMD_M128I *)&l, sL8A);
    if constexpr (stereo)
        SIMD_MM(store_si128)((SIMD_M128I *)&r, sR8A);
    l[0] = (l[0] + l[1]) + (l[2] + l[3]);
    if constexpr (stereo)
        r[0] = (r[0] + r[1]) + (r[2] + r[3]);

    fL = SIMD_MM(mul_ss)(SIMD_MM(cvtsi32_ss)(fL, l[0]), I16InvScale_m128);
    if constexpr (stereo)
        fR = SIMD_MM(mul_ss)(SIMD_MM(cvtsi32_ss)(fR, r[0]), I16InvScale_m128);

    SIMD_MM(store_ss)(&OutputL[i], fL);
    if constexpr (stereo)
        SIMD_MM(store_ss)(&OutputR[i], fR);

    if constexpr (LOOP_ACTIVE)
    {
        if (ks.fadeActive)
        {
            int16_t *readFadeSampleR{nullptr};
            if constexpr (stereo)
            {
                readFadeSampleR = ks.ReadFadeSample[1];
            }

            sL8A = SIMD_MM(madd_epi16)(tmp, SIMD_MM(loadu_si128)((SIMD_M128I *)readFadeSampleL));
            if constexpr (stereo)
                sR8A =
                    SIMD_MM(madd_epi16)(tmp, SIMD_MM(loadu_si128)((SIMD_M128I *)readFadeSampleR));
            sL8B = SIMD_MM(madd_epi16)(tmp2,
                                       SIMD_MM(loadu_si128)((SIMD_M128I *)(readFadeSampleL + 8)));
            if constexpr (stereo)
                sR8B = SIMD_MM(madd_epi16)(
                    tmp2, SIMD_MM(loadu_si128)((SIMD_M128I *)(readFadeSampleR + 8)));

            sL8A = SIMD_MM(add_epi32)(sL8A, sL8B);
            if constexpr (stereo)
                sR8A = SIMD_MM(add_epi32)(sR8A, sR8B);

            int l alignas(16)[4], r alignas(16)[4];
            SIMD_MM(store_si128)((SIMD_M128I *)&l, sL8A);
            if constexpr (stereo)
                SIMD_MM(store_si128)((SIMD_M128I *)&r, sR8A);
            l[0] = (l[0] + l[1]) + (l[2] + l[3]);
            if constexpr (stereo)
                r[0] = (r[0] + r[1]) + (r[2] + r[3]);

            fL = SIMD_MM(mul_ss)(SIMD_MM(cvtsi32_ss)(fL, l[0]), I16InvScale_m128);
            if constexpr (stereo)
                fR = SIMD_MM(mul_ss)(SIMD_MM(cvtsi32_ss)(fR, r[0]), I16InvScale_m128);

            float fadeValL{0.f};
            float fadeValR{0.f};
            SIMD_MM(store_ss)(&fadeValL, fL);
            if constexpr (stereo)
                SIMD_MM(store_ss)(&fadeValR, fR);

            auto fadeGain(
                getFadeGain(ks.SamplePos, GD->loopUpperBound - ks.loopFade, GD->loopUpperBound));

            auto aOut = getFadeGainToAmp(1.f - fadeGain);
            fadeGain = getFadeGainToAmp(fadeGain);

            OutputL[i] = OutputL[i] * aOut + fadeValL * fadeGain;
            if constexpr (stereo)
                OutputR[i] = OutputR[i] * aOut + fadeValR * fadeGain;
        }
    }
}

template <typename T>
template <int NUM_CHANNELS, bool LOOP_ACTIVE>
void KernelOp<InterpolationTypes::Linear, T>::Process(
    GeneratorState *__restrict GD,
    KernelProcessor<InterpolationTypes::Linear, T, NUM_CHANNELS, LOOP_ACTIVE> &ks)
{
    auto readSampleL{ks.ReadSample[0]};
    auto readFadeSampleL{ks.ReadFadeSample[0]};
    auto OutputL{ks.Output[0]};
    auto SamplePos{ks.SamplePos};
    auto m0{ks.m0};
    auto i{ks.i};

    auto f_subPos = (float)(ks.SampleSubPos);
    f_subPos /= (1 << 24);

    auto readPos = FIRoffset - 1;
    auto y0{NormalizeSampleToF32(readSampleL[readPos])};
    auto y1{NormalizeSampleToF32(readSampleL[readPos + 1])};

    OutputL[i] = (y0 * (1 - f_subPos) + y1 * (f_subPos));

    if constexpr (LOOP_ACTIVE)
    {
        if (ks.fadeActive)
        {
            auto fadeVal0{NormalizeSampleToF32(readFadeSampleL[readPos])};
            auto fadeVal1{NormalizeSampleToF32(readFadeSampleL[readPos + 1])};
            auto fadeVal = fadeVal0 * (1 - f_subPos) + fadeVal1 * f_subPos;
            auto fadeGain(
                getFadeGain(ks.SamplePos, GD->loopUpperBound - ks.loopFade, GD->loopUpperBound));
            auto aOut = getFadeGainToAmp(1.f - fadeGain);
            fadeGain = getFadeGainToAmp(fadeGain);

            OutputL[i] = OutputL[i] * aOut + fadeVal * fadeGain;
        }
    }

    if constexpr (NUM_CHANNELS == 2)
    {
        auto readSampleR{ks.ReadSample[1]};
        auto readFadeSampleR{ks.ReadFadeSample[1]};
        auto OutputR{ks.Output[1]};

        auto y0{NormalizeSampleToF32(readSampleR[readPos])};
        auto y1{NormalizeSampleToF32(readSampleR[readPos + 1])};

        OutputR[i] = (y0 * (1 - f_subPos) + y1 * (f_subPos));

        if constexpr (LOOP_ACTIVE)
        {
            if (ks.fadeActive)
            {
                float fadeVal0{NormalizeSampleToF32(readFadeSampleR[readPos])};
                float fadeVal1{NormalizeSampleToF32(readFadeSampleR[readPos + 1])};
                auto fadeVal = fadeVal0 * (1 - f_subPos) + fadeVal1 * f_subPos;

                auto fadeGain(getFadeGain(ks.SamplePos, GD->loopUpperBound - ks.loopFade,
                                          GD->loopUpperBound));
                auto aOut = getFadeGainToAmp(1.f - fadeGain);
                fadeGain = getFadeGainToAmp(fadeGain);

                OutputR[i] = OutputR[i] * aOut + fadeVal * fadeGain;
            }
        }
    }
}

template <int NUM_CHANNELS, bool LOOP_ACTIVE>
void KernelOp<InterpolationTypes::Sinc, float>::Process(
    GeneratorState *__restrict GD,
    KernelProcessor<InterpolationTypes::Sinc, float, NUM_CHANNELS, LOOP_ACTIVE> &ks)
{
    auto readSampleL{ks.ReadSample[0]};
    auto readFadeSampleL{ks.ReadFadeSample[0]};
    auto OutputL{ks.Output[0]};
    auto m0{ks.m0};
    auto i{ks.i};

#if DEBUG_CODE_I_WANT_TO_RETAIN
    /* This code only works if theres no loop etc but please leave it here
     * since it is useful when debugging in the future
     */
    {
        ptrdiff_t space = (float *)readSampleL - (float *)ks.IO->sampleDataL;
        float above = space - ks.IO->waveSize + FIRipol_N;

        if (space < -(int64_t)FIRoffset || above > FIRoffset)
            SCLOG(SCD(ks.IO->sampleDataL)
                  << SCD(readSampleL) << SCD(space) << SCD(above) << SCD(ks.IO->waveSize));

        assert(space >= -(int64_t)FIRoffset && above <= FIRoffset);
    }
#endif

    // float32 path (SSE)
    SIMD_M128 lipol0, tmp[4], sL4, sR4;
    lipol0 = SIMD_MM(setzero_ps)();
    lipol0 = SIMD_MM(cvtsi32_ss)(lipol0, ks.SampleSubPos & 0xffff);
    lipol0 = SIMD_MM(shuffle_ps)(lipol0, lipol0, SIMD_MM_SHUFFLE(0, 0, 0, 0));
    tmp[0] = SIMD_MM(add_ps)(SIMD_MM(mul_ps)(*((SIMD_M128 *)&sincTable.SincOffsetF32[m0]), lipol0),
                             *((SIMD_M128 *)&sincTable.SincTableF32[m0]));
    tmp[1] =
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(*((SIMD_M128 *)&sincTable.SincOffsetF32[m0 + 4]), lipol0),
                        *((SIMD_M128 *)&sincTable.SincTableF32[m0 + 4]));
    tmp[2] =
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(*((SIMD_M128 *)&sincTable.SincOffsetF32[m0 + 8]), lipol0),
                        *((SIMD_M128 *)&sincTable.SincTableF32[m0 + 8]));
    tmp[3] =
        SIMD_MM(add_ps)(SIMD_MM(mul_ps)(*((SIMD_M128 *)&sincTable.SincOffsetF32[m0 + 12]), lipol0),
                        *((SIMD_M128 *)&sincTable.SincTableF32[m0 + 12]));
    sL4 = SIMD_MM(mul_ps)(tmp[0], SIMD_MM(loadu_ps)(readSampleL));
    sL4 = SIMD_MM(add_ps)(sL4, SIMD_MM(mul_ps)(tmp[1], SIMD_MM(loadu_ps)(readSampleL + 4)));
    sL4 = SIMD_MM(add_ps)(sL4, SIMD_MM(mul_ps)(tmp[2], SIMD_MM(loadu_ps)(readSampleL + 8)));
    sL4 = SIMD_MM(add_ps)(sL4, SIMD_MM(mul_ps)(tmp[3], SIMD_MM(loadu_ps)(readSampleL + 12)));
    // sL4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sL4);
    sL4 = SIMD_MM(hadd_ps)(sL4, sL4);
    sL4 = SIMD_MM(hadd_ps)(sL4, sL4);

    SIMD_MM(store_ss)(&OutputL[i], sL4);

    if constexpr (LOOP_ACTIVE)
    {
        if (ks.fadeActive)
        {
            sR4 = SIMD_MM(mul_ps)(tmp[0], SIMD_MM(loadu_ps)(readFadeSampleL));
            sR4 = SIMD_MM(add_ps)(sR4,
                                  SIMD_MM(mul_ps)(tmp[1], SIMD_MM(loadu_ps)(readFadeSampleL + 4)));
            sR4 = SIMD_MM(add_ps)(sR4,
                                  SIMD_MM(mul_ps)(tmp[2], SIMD_MM(loadu_ps)(readFadeSampleL + 8)));
            sR4 = SIMD_MM(add_ps)(sR4,
                                  SIMD_MM(mul_ps)(tmp[3], SIMD_MM(loadu_ps)(readFadeSampleL + 12)));
            // sR4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sR4);
            sR4 = SIMD_MM(hadd_ps)(sR4, sR4);
            sR4 = SIMD_MM(hadd_ps)(sR4, sR4);

            float fadeVal{0.f};
            SIMD_MM(store_ss)(&fadeVal, sR4);
            auto fadeGain(
                getFadeGain(ks.SamplePos, GD->loopUpperBound - ks.loopFade, GD->loopUpperBound));
            auto aOut = getFadeGainToAmp(1.f - fadeGain);
            fadeGain = getFadeGainToAmp(fadeGain);

            OutputL[i] = OutputL[i] * aOut + fadeVal * fadeGain;
        }
    }

    if constexpr (NUM_CHANNELS == 2)
    {
        auto readSampleR{ks.ReadSample[1]};
        auto readFadeSampleR{ks.ReadFadeSample[1]};
        auto OutputR{ks.Output[1]};

        sR4 = SIMD_MM(mul_ps)(tmp[0], SIMD_MM(loadu_ps)(readSampleR));
        sR4 = SIMD_MM(add_ps)(sR4, SIMD_MM(mul_ps)(tmp[1], SIMD_MM(loadu_ps)(readSampleR + 4)));
        sR4 = SIMD_MM(add_ps)(sR4, SIMD_MM(mul_ps)(tmp[2], SIMD_MM(loadu_ps)(readSampleR + 8)));
        sR4 = SIMD_MM(add_ps)(sR4, SIMD_MM(mul_ps)(tmp[3], SIMD_MM(loadu_ps)(readSampleR + 12)));
        // sR4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sR4);
        sR4 = SIMD_MM(hadd_ps)(sR4, sR4);
        sR4 = SIMD_MM(hadd_ps)(sR4, sR4);

        SIMD_MM(store_ss)(&OutputR[i], sR4);

        if constexpr (LOOP_ACTIVE)
        {
            if (ks.fadeActive)
            {
                sR4 = SIMD_MM(mul_ps)(tmp[0], SIMD_MM(loadu_ps)(readFadeSampleR));
                sR4 = SIMD_MM(add_ps)(
                    sR4, SIMD_MM(mul_ps)(tmp[1], SIMD_MM(loadu_ps)(readFadeSampleR + 4)));
                sR4 = SIMD_MM(add_ps)(
                    sR4, SIMD_MM(mul_ps)(tmp[2], SIMD_MM(loadu_ps)(readFadeSampleR + 8)));
                sR4 = SIMD_MM(add_ps)(
                    sR4, SIMD_MM(mul_ps)(tmp[3], SIMD_MM(loadu_ps)(readFadeSampleR + 12)));
                // sR4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sR4);
                sR4 = SIMD_MM(hadd_ps)(sR4, sR4);
                sR4 = SIMD_MM(hadd_ps)(sR4, sR4);

                float fadeVal{0.f};
                SIMD_MM(store_ss)(&fadeVal, sR4);
                auto fadeGain(getFadeGain(ks.SamplePos, GD->loopUpperBound - ks.loopFade,
                                          GD->loopUpperBound));
                auto aOut = getFadeGainToAmp(1.f - fadeGain);
                fadeGain = getFadeGainToAmp(fadeGain);

                OutputR[i] = OutputR[i] * aOut + fadeVal * fadeGain;
            }
        }
    }
}

template <int NUM_CHANNELS, bool LOOP_ACTIVE>
void KernelOp<InterpolationTypes::Sinc, int16_t>::Process(
    GeneratorState *__restrict GD,
    KernelProcessor<InterpolationTypes::Sinc, int16_t, NUM_CHANNELS, LOOP_ACTIVE> &ks)
{
    auto readSampleL{ks.ReadSample[0]};
    auto readFadeSampleL{ks.ReadFadeSample[0]};
    auto OutputL{ks.Output[0]};
    auto m0{ks.m0};
    auto i{ks.i};
    constexpr auto stereo = NUM_CHANNELS == 2;

    int16_t *readSampleR{nullptr};
    float *OutputR{nullptr};
    if constexpr (stereo)
    {
        readSampleR = ks.ReadSample[1];
        OutputR = ks.Output[1];
    }

    // int16
    // SSE2 path
    SIMD_M128I lipol0, tmp, sL8A, sR8A, tmp2, sL8B, sR8B;
    auto fL = SIMD_MM(setzero_ps)(), fR = SIMD_MM(setzero_ps)();
    lipol0 = SIMD_MM(set1_epi16)(ks.SampleSubPos & 0xffff);

    tmp = SIMD_MM(add_epi16)(
        SIMD_MM(mulhi_epi16)(*((SIMD_M128I *)&sincTable.SincOffsetI16[m0]), lipol0),
        *((SIMD_M128I *)&sincTable.SincTableI16[m0]));
    sL8A = SIMD_MM(madd_epi16)(tmp, SIMD_MM(loadu_si128)((SIMD_M128I *)readSampleL));
    if constexpr (stereo)
        sR8A = SIMD_MM(madd_epi16)(tmp, SIMD_MM(loadu_si128)((SIMD_M128I *)readSampleR));

    tmp2 = SIMD_MM(add_epi16)(
        SIMD_MM(mulhi_epi16)(*((SIMD_M128I *)&sincTable.SincOffsetI16[m0 + 8]), lipol0),
        *((SIMD_M128I *)&sincTable.SincTableI16[m0 + 8]));
    sL8B = SIMD_MM(madd_epi16)(tmp2, SIMD_MM(loadu_si128)((SIMD_M128I *)(readSampleL + 8)));
    if constexpr (stereo)
        sR8B = SIMD_MM(madd_epi16)(tmp2, SIMD_MM(loadu_si128)((SIMD_M128I *)(readSampleR + 8)));

    sL8A = SIMD_MM(add_epi32)(sL8A, sL8B);
    if constexpr (stereo)
        sR8A = SIMD_MM(add_epi32)(sR8A, sR8B);

    int l alignas(16)[4], r alignas(16)[4];
    SIMD_MM(store_si128)((SIMD_M128I *)&l, sL8A);
    if constexpr (stereo)
        SIMD_MM(store_si128)((SIMD_M128I *)&r, sR8A);
    l[0] = (l[0] + l[1]) + (l[2] + l[3]);
    if constexpr (stereo)
        r[0] = (r[0] + r[1]) + (r[2] + r[3]);

    fL = SIMD_MM(mul_ss)(SIMD_MM(cvtsi32_ss)(fL, l[0]), I16InvScale_m128);
    if constexpr (stereo)
        fR = SIMD_MM(mul_ss)(SIMD_MM(cvtsi32_ss)(fR, r[0]), I16InvScale_m128);

    SIMD_MM(store_ss)(&OutputL[i], fL);
    if constexpr (stereo)
        SIMD_MM(store_ss)(&OutputR[i], fR);

    if constexpr (LOOP_ACTIVE)
    {
        if (ks.fadeActive)
        {
            int16_t *readFadeSampleR{nullptr};
            if constexpr (stereo)
            {
                readFadeSampleR = ks.ReadFadeSample[1];
            }

            sL8A = SIMD_MM(madd_epi16)(tmp, SIMD_MM(loadu_si128)((SIMD_M128I *)readFadeSampleL));
            if constexpr (stereo)
                sR8A =
                    SIMD_MM(madd_epi16)(tmp, SIMD_MM(loadu_si128)((SIMD_M128I *)readFadeSampleR));
            sL8B = SIMD_MM(madd_epi16)(tmp2,
                                       SIMD_MM(loadu_si128)((SIMD_M128I *)(readFadeSampleL + 8)));
            if constexpr (stereo)
                sR8B = SIMD_MM(madd_epi16)(
                    tmp2, SIMD_MM(loadu_si128)((SIMD_M128I *)(readFadeSampleR + 8)));

            sL8A = SIMD_MM(add_epi32)(sL8A, sL8B);
            if constexpr (stereo)
                sR8A = SIMD_MM(add_epi32)(sR8A, sR8B);

            int l alignas(16)[4], r alignas(16)[4];
            SIMD_MM(store_si128)((SIMD_M128I *)&l, sL8A);
            if constexpr (stereo)
                SIMD_MM(store_si128)((SIMD_M128I *)&r, sR8A);
            l[0] = (l[0] + l[1]) + (l[2] + l[3]);
            if constexpr (stereo)
                r[0] = (r[0] + r[1]) + (r[2] + r[3]);

            fL = SIMD_MM(mul_ss)(SIMD_MM(cvtsi32_ss)(fL, l[0]), I16InvScale_m128);
            if constexpr (stereo)
                fR = SIMD_MM(mul_ss)(SIMD_MM(cvtsi32_ss)(fR, r[0]), I16InvScale_m128);

            float fadeValL{0.f};
            float fadeValR{0.f};
            SIMD_MM(store_ss)(&fadeValL, fL);
            if constexpr (stereo)
                SIMD_MM(store_ss)(&fadeValR, fR);

            auto fadeGain(
                getFadeGain(ks.SamplePos, GD->loopUpperBound - ks.loopFade, GD->loopUpperBound));

            auto aOut = getFadeGainToAmp(1.f - fadeGain);
            fadeGain = getFadeGainToAmp(fadeGain);

            OutputL[i] = OutputL[i] * aOut + fadeValL * fadeGain;
            if constexpr (stereo)
                OutputR[i] = OutputR[i] * aOut + fadeValR * fadeGain;
        }
    }
}

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
    return detail::generatorGet(loopValue, std::make_index_sequence<(1 << 5)>());
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

    int loopFade = std::min(GD->loopFade, GD->loopLowerBound - GD->playbackLowerBound);
    loopFade = std::min(loopFade, GD->loopUpperBound - GD->loopLowerBound);

    bool fadeActive =
        SamplePos > (GD->loopUpperBound - loopFade) && SamplePos <= GD->loopUpperBound;

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
    int16_t *__restrict readFadeSampleL = nullptr;
    int16_t *__restrict readFadeSampleR = nullptr;
    int16_t loopEndBufferL[resampFIRSize], loopEndBufferR[resampFIRSize];
    float *__restrict readSampleLF32 = nullptr;
    float *__restrict readSampleRF32 = nullptr;
    float *__restrict readFadeSampleLF32 = nullptr;
    float *__restrict readFadeSampleRF32 = nullptr;
    float loopEndBufferLF32[resampFIRSize], loopEndBufferRF32[resampFIRSize];

    if (fp)
    {
        // See comment above - the generator wants an FIRoffset centered data set
        readSampleLF32 = SampleDataFL + SamplePos - FIRoffset;
        if (stereo)
            readSampleRF32 = SampleDataFR + SamplePos - FIRoffset;

        if constexpr (loopActive)
        {
            if (fadeActive)
            {
                auto fadeSamplePos{GD->loopLowerBound - (GD->loopUpperBound - SamplePos)};
                readFadeSampleLF32 = SampleDataFL + fadeSamplePos - FIRoffset;
                if (stereo)
                    readFadeSampleRF32 = SampleDataFR + fadeSamplePos - FIRoffset;
            }

            if (SamplePos >= WaveSize - resampFIRSize && SamplePos <= GD->loopUpperBound)
            {
                for (int k = 0; k < resampFIRSize; ++k)
                {
                    auto q = k + SamplePos - FIRoffset;
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
        readSampleL = SampleDataL + SamplePos - FIRoffset;
        if (stereo)
            readSampleR = SampleDataR + SamplePos - FIRoffset;

        if constexpr (loopActive)
        {
            if (fadeActive)
            {
                auto fadeSamplePos{GD->loopLowerBound - (GD->loopUpperBound - SamplePos)};
                readFadeSampleL = SampleDataL + fadeSamplePos - FIRoffset;
                if (stereo)
                    readFadeSampleR = SampleDataR + fadeSamplePos - FIRoffset;
            }

            if (SamplePos >= WaveSize - resampFIRSize && SamplePos <= GD->loopUpperBound)
            {
                for (int k = 0; k < resampFIRSize; ++k)
                {
                    auto q = k + SamplePos - FIRoffset;
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
#define KPStereo(E, T, C, dataL, dataR, fadeL, fadeR)                                              \
    KernelProcessor<E, T, C, loopActive> kp{                                                       \
        SamplePos,  SampleSubPos, int32_t(m0),        i, {dataL, dataR}, {fadeL, fadeR},           \
        fadeActive, loopFade,     {OutputL, OutputR}, IO};                                         \
    kp.ProcessKernel(GD);

#define KPMono(E, T, C, data, fade)                                                                \
    KernelProcessor<E, T, C, loopActive> ks{                                                       \
        SamplePos, SampleSubPos, int32_t(m0), i,         {data},                                   \
        {fade},    fadeActive,   loopFade,    {OutputL}, IO};                                      \
    ks.ProcessKernel(GD);

        using type_from_cond = typename std::conditional<fp, float, int16_t>::type;
        type_from_cond *readL, *readFadeL, *readR, *readFadeR;
        if constexpr (fp)
        {
            readL = readSampleLF32;
            readR = readSampleRF32;
            readFadeL = readFadeSampleLF32;
            readFadeR = readFadeSampleRF32;
        }
        else
        {
            readL = readSampleL;
            readR = readSampleR;
            readFadeL = readFadeSampleL;
            readFadeR = readFadeSampleR;
        }

        // 2. Resample
        unsigned int m0 = ((SampleSubPos >> 12) & 0xff0);
        if (stereo)
        {
            switch (GD->interpolationType)
            {
            case InterpolationTypes::Sinc:
            {
                KPStereo(InterpolationTypes::Sinc, type_from_cond, 2, readL, readR, readFadeL,
                         readFadeR);
                break;
            }
            case InterpolationTypes::Linear:
            {
                KPStereo(InterpolationTypes::Linear, type_from_cond, 2, readL, readR, readFadeL,
                         readFadeR);
                break;
            }
            case InterpolationTypes::ZeroOrderHoldAA:
            {
                KPStereo(InterpolationTypes::ZeroOrderHoldAA, type_from_cond, 2, readL, readR,
                         readFadeL, readFadeR);
                break;
            }
            case InterpolationTypes::ZeroOrderHold:
            {
                KPStereo(InterpolationTypes::ZeroOrderHold, type_from_cond, 2, readL, readR,
                         readFadeL, readFadeR);
                break;
            }
            }
        }
        else
        {
            switch (GD->interpolationType)
            {
            case InterpolationTypes::Sinc:
            {
                KPMono(InterpolationTypes::Sinc, type_from_cond, 1, readL, readFadeL);
                break;
            }
            case InterpolationTypes::Linear:
            {
                KPMono(InterpolationTypes::Linear, type_from_cond, 1, readL, readFadeL);
                break;
            }
            case InterpolationTypes::ZeroOrderHoldAA:
            {
                KPMono(InterpolationTypes::ZeroOrderHoldAA, type_from_cond, 1, readL, readFadeL);
                break;
            }
            case InterpolationTypes::ZeroOrderHold:
            {
                KPMono(InterpolationTypes::ZeroOrderHold, type_from_cond, 1, readL, readFadeL);
                break;
            }
            }
        }

#define DEBUG_OUTPUT_MINMAX 0
#if DEBUG_OUTPUT_MINMAX
        {
            // Please don't remove this in some cleanup. It is handly
            static int printEvery{0};
            static float mxOut = std::numeric_limits<float>::min();
            static float mnOut = std::numeric_limits<float>::max();

            mxOut = std::max(OutputL[i], mxOut);
            mnOut = std::min(OutputL[i], mnOut);
            if (printEvery == 1000)
            {
                SCLOG("GENERATOR " << SCD(mxOut) << " " << SCD(mnOut));
                printEvery = 0;
                mxOut = std::numeric_limits<float>::min();
                mnOut = std::numeric_limits<float>::max();
            }
            printEvery++;
        }
#endif

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
                readSampleLF32 = SampleDataFL + SamplePos - FIRoffset;
                if (stereo)
                    readSampleRF32 = SampleDataFR + SamplePos - FIRoffset;
            }
            else
            {
                readSampleL = SampleDataL + SamplePos - FIRoffset;
                if (stereo)
                    readSampleR = SampleDataR + SamplePos - FIRoffset;
            }
        }
        else if constexpr (!loopWhileGated && loopForward)
        {
            int offset = SamplePos;

            if (Direction > 0)
            {
                // Upper
                if (offset > GD->loopUpperBound)
                {
                    offset -= LoopOffset;
                    GD->loopCount++;
                }
            }
            else
            {
                // Lower
                if (offset < GD->loopLowerBound)
                {
                    offset += LoopOffset;
                    GD->loopCount++;
                }
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
                if (GD->directionAtOutset == -1 && Direction == 1)
                    GD->loopCount++;
                Direction = -1;
            }
            else if (SamplePos <= GD->loopLowerBound)
            {
                if (GD->directionAtOutset == 1 && Direction == -1)
                    GD->loopCount++;
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
                    {
                        offset -= LoopOffset;
                        GD->loopCount++;
                    }
                }
                else
                {
                    if (offset < GD->loopLowerBound)
                    {
                        offset += LoopOffset;
                        GD->loopCount++;
                    }
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
                    if (GD->directionAtOutset == -1 && Direction == 1)
                        GD->loopCount++;
                    Direction = -1;
                }
                else if (SamplePos <= GD->loopLowerBound)
                {
                    if (GD->directionAtOutset == 1 && Direction == -1)
                        GD->loopCount++;
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
                        auto q = k + SamplePos - FIRoffset;
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
                    readSampleLF32 = SampleDataFL + SamplePos - FIRoffset;
                    if (stereo)
                        readSampleRF32 = SampleDataFR + SamplePos - FIRoffset;

                    if (fadeActive)
                    {
                        auto fadeSamplePos{GD->loopLowerBound - (GD->loopUpperBound - SamplePos)};
                        readFadeSampleLF32 = SampleDataFL + fadeSamplePos - FIRoffset;
                        if (stereo)
                            readFadeSampleRF32 = SampleDataFR + fadeSamplePos - FIRoffset;
                    }
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
                        auto q = k + SamplePos - FIRoffset;
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
                    readSampleL = SampleDataL + SamplePos - FIRoffset;
                    if (stereo)
                        readSampleR = SampleDataR + SamplePos - FIRoffset;

                    if (fadeActive)
                    {
                        auto fadeSamplePos{GD->loopLowerBound - (GD->loopUpperBound - SamplePos)};
                        readFadeSampleL = SampleDataL + fadeSamplePos - FIRoffset;
                        if (stereo)
                            readFadeSampleR = SampleDataR + fadeSamplePos - FIRoffset;
                    }
                }
            }
        }

        if constexpr (loopActive)
        {
            fadeActive = fadeActive && (SamplePos > (GD->loopUpperBound - loopFade)) &&
                         (SamplePos <= GD->loopUpperBound);
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
        if (GD->isInLoop && GD->loopCount < 0)
            GD->loopCount = 0;
    }
}
} // namespace scxt::dsp
