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
#include <utility>
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

template <InterpolationTypes KT, typename T> struct KernelOp
{
};

/* These all have that defaulted bool argument
 so that we can avoid duplicating the whole sinc code another two times for the anti-aliased
 zero-order hold version. Nothing ever actually sets it except for the KernelProcessor.
 See comment in the definition below.
 */

template <InterpolationTypes KT, typename T, int NUM_CHANNELS, bool LOOP_ACTIVE>
struct KernelProcessor;

template <typename T> struct KernelOp<InterpolationTypes::ZeroOrderHold, T>
{
    template <int NUM_CHANNELS, bool LOOP_ACTIVE>
    static void
    Process(GeneratorState *__restrict GD,
            KernelProcessor<InterpolationTypes::ZeroOrderHold, T, NUM_CHANNELS, LOOP_ACTIVE> &ks);
};

template <typename T> struct KernelOp<InterpolationTypes::ZOHAA, T>
{
    template <int NUM_CHANNELS, bool LOOP_ACTIVE>
    static void
    Process(GeneratorState *__restrict GD,
            KernelProcessor<InterpolationTypes::ZOHAA, T, NUM_CHANNELS, LOOP_ACTIVE> &ks);
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
    // the gain law lives in the harness, since it differs per loop shape; a kernel
    // just mixes its two reads
    float mainGain, partnerGain;

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
            OutputL[i] = OutputL[i] * ks.mainGain + fadeVal * ks.partnerGain;
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
                OutputR[i] = OutputR[i] * ks.mainGain + fadeVal * ks.partnerGain;
            }
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
            OutputL[i] = OutputL[i] * ks.mainGain + fadeVal * ks.partnerGain;
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

                OutputR[i] = OutputR[i] * ks.mainGain + fadeVal * ks.partnerGain;
            }
        }
    }
}

template <typename T>
template <int NUM_CHANNELS, bool LOOP_ACTIVE>
void KernelOp<InterpolationTypes::ZOHAA, T>::Process(
    GeneratorState *__restrict GD,
    KernelProcessor<InterpolationTypes::ZOHAA, T, NUM_CHANNELS, LOOP_ACTIVE> &ks)
{
    auto readSampleL{ks.ReadSample[0]};
    auto readFadeSampleL{ks.ReadFadeSample[0]};
    auto OutputL{ks.Output[0]};
    auto SamplePos{ks.SamplePos};
    auto m0{ks.m0};
    auto i{ks.i};

    auto f_subPos = (float)(ks.SampleSubPos) / (float)(1 << 24);
    auto subRatio = std::abs((float)(GD->ratio) / (float)(1 << 24));
    f_subPos = std::pow(f_subPos, 0.5f * subRatio + 0.5f / subRatio);

    auto readPos = FIRoffset - 2;

    // The crossfade partner has to be interpolated exactly like the main read. It used
    // to be a linear blend of [readPos, readPos+1] - one sample early, since the cubic
    // sits between readPos+1 and readPos+2 - using the already pow-warped subposition.
    auto cubic = [readPos, f_subPos](const T *__restrict src) {
        auto y0{NormalizeSampleToF32(src[readPos])};
        auto y1{NormalizeSampleToF32(src[readPos + 1])};
        auto y2{NormalizeSampleToF32(src[readPos + 2])};
        auto y3{NormalizeSampleToF32(src[readPos + 3])};
        auto a = ((3.f * (y1 - y2)) - y0 + y3) * 0.5f;
        auto b = y2 + y2 + y0 - (5.f * y1 + y3) * 0.5f;
        auto c = (y2 - y0) * 0.5f;
        return ((a * f_subPos + b) * f_subPos + c) * f_subPos + y1;
    };

    OutputL[i] = cubic(readSampleL);

    if constexpr (LOOP_ACTIVE)
    {
        if (ks.fadeActive)
            OutputL[i] = OutputL[i] * ks.mainGain + cubic(readFadeSampleL) * ks.partnerGain;
    }

    if constexpr (NUM_CHANNELS == 2)
    {
        auto readSampleR{ks.ReadSample[1]};
        auto readFadeSampleR{ks.ReadFadeSample[1]};
        auto OutputR{ks.Output[1]};

        OutputR[i] = cubic(readSampleR);

        if constexpr (LOOP_ACTIVE)
        {
            if (ks.fadeActive)
                OutputR[i] = OutputR[i] * ks.mainGain + cubic(readFadeSampleR) * ks.partnerGain;
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

#if DEBUG_GENERATOR
    /* This code only works if theres no loop etc but please leave it here
     * since it is useful when debugging in the future
     */
    {
        ptrdiff_t space = (float *)readSampleL - (float *)ks.IO->sampleDataL;
        float above = space - ks.IO->waveSize + FIRipol_N;

        if (space < -(int64_t)FIRoffset || above > FIRoffset)
            SCLOG_IF(debug, SCD(ks.IO->sampleDataL) << SCD(readSampleL) << SCD(space) << SCD(above)
                                                    << SCD(ks.IO->waveSize));

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
            OutputL[i] = OutputL[i] * ks.mainGain + fadeVal * ks.partnerGain;
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
                OutputR[i] = OutputR[i] * ks.mainGain + fadeVal * ks.partnerGain;
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

            OutputL[i] = OutputL[i] * ks.mainGain + fadeValL * ks.partnerGain;
            if constexpr (stereo)
                OutputR[i] = OutputR[i] * ks.mainGain + fadeValR * ks.partnerGain;
        }
    }
}

// mirrors a playhead that ran past a ping-pong bound back in, however many times it crossed
// the turn points, which a crossfade moves half a fade below the loop bounds
inline void reflectPingPong(int &samplePos, int &sampleSubPos, int &travel, int ratioSign,
                            int64_t ratio, int mirrorLo, int mirrorHi, GeneratorState *GD)
{
    const int64_t lo = (int64_t)mirrorLo << 24;
    const int64_t len = (int64_t)std::max(1, mirrorHi - mirrorLo) << 24;
    const int64_t hi = lo + len;
    int64_t pos = ((int64_t)samplePos << 24) + sampleSubPos;

    auto over = travel > 0 ? pos - hi : lo - pos;
    if (over < 0)
        return;

    GD->hasLooped = true;

    if (over > ratio)
    {
        // already outside before this step, so head back rather than jump in
        travel = -travel;
        return;
    }

    const auto trips = over / (2 * len);
    const auto phase = over % (2 * len);
    const auto travelBefore = travel;
    if (phase < len)
    {
        pos = travel > 0 ? hi - phase : lo + phase;
        travel = -travel;
    }
    else
    {
        pos = travel > 0 ? lo + (phase - len) : hi - (phase - len);
    }

    // a loop counts when the playhead turns at the bound it set out from, and
    // directionAtOutset is a loop direction, so fold the ratio sign back out first
    const auto turnsAtFirstBound = trips + 1;
    const auto turnsAtOtherBound = trips + (phase >= len ? 1 : 0);
    GD->loopCount +=
        (int16_t)(travelBefore * ratioSign != GD->directionAtOutset ? turnsAtFirstBound
                                                                    : turnsAtOtherBound);

    samplePos = (int)(pos >> 24);
    sampleSubPos = (int)(pos & ((1 << 24) - 1));
}

// sample index for a window straddling the loop end, wrapped back into the loop
inline int loopEndIndex(int k, int samplePos, int loopUpperBound, int waveSize, int loopLength)
{
    // signed, since a short loop's window starts in the pad before the sample
    int q = k + samplePos - (int)FIRoffset;
    const auto top = std::min(loopUpperBound, waveSize);
    if (q >= top)
        q -= loopLength * ((q - top) / loopLength + 1);
    return std::max(q, -(int)FIRoffset);
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
    const int RatioSign = GD->ratio < 0 ? -1 : 1;
    const int64_t Ratio = std::abs(GD->ratio);

    // shared with the editor so the XF value on screen is the one you hear
    const int loopFade = (int)clampLoopFade(GD->loopFade, GD->playbackLowerBound,
                                            GD->loopLowerBound, GD->loopUpperBound);
    const int fadeLo = GD->loopUpperBound - loopFade;

    // a ping-pong fade fills the loopFade samples up to each bound rather than straddling
    // it, which puts the turn half a fade inside the loop
    const int fadeHalf = loopForward ? 0 : loopFade / 2;
    const int mirrorLo = GD->loopLowerBound - fadeHalf;
    const int mirrorHi = GD->loopUpperBound - fadeHalf;

    /*
     * Both directions are named locals and are kept in lockstep by reflect(). Travel is
     * what moves the playhead and what decides we have run off the end; LoopDir is what
     * the loop logic reasons about and what directionAtOutset compares against. See the
     * comment on GeneratorState.
     */
    int Travel = GD->loopDirection * RatioSign;
    int LoopDir = GD->loopDirection;
    auto reflect = [&]() {
        reflectPingPong(SamplePos, SampleSubPos, Travel, RatioSign, Ratio, mirrorLo, mirrorHi, GD);
        LoopDir = Travel * RatioSign;
    };
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
    int16_t *__restrict readFadeSampleL = nullptr;
    int16_t *__restrict readFadeSampleR = nullptr;
    int16_t loopEndBufferL[resampFIRSize], loopEndBufferR[resampFIRSize];
    float *__restrict readSampleLF32 = nullptr;
    float *__restrict readSampleRF32 = nullptr;
    float *__restrict readFadeSampleLF32 = nullptr;
    float *__restrict readFadeSampleRF32 = nullptr;
    float loopEndBufferLF32[resampFIRSize], loopEndBufferRF32[resampFIRSize];

    /*
     * Is the loop still what is driving playback? Once a gated loop is released the
     * playhead runs on past loopUpperBound into the tail and there is no seam left to
     * smooth, so the crossfade has to stop rather than blend in unrelated pre-loop
     * material on the way out.
     */
    auto loopIsContinuing = [&]() -> bool {
        if constexpr (loopWhileGated)
            return GD->gated || (LoopDir != GD->directionAtOutset);
        else
            return true;
    };

    /*
     * Where the crossfade is, what it blends against, and how far through it we are.
     *
     * This is a pure function of position and direction, evaluated identically at block
     * entry and after every advance. It used to be a bool computed once per block and
     * from then on only ever cleared, so a fade could not begin in the middle of a block.
     * A forward loop approaches its seam through the window and therefore enters at gain
     * ~0, where starting a fraction of a block late is nearly inaudible. Reverse playback
     * leaves the seam through the window and has to engage at gain ~1 the instant it
     * wraps, which the old code could not do - so every wrap emitted pure loop tail until
     * the next block boundary and then jumped to the partner. That is the click in #2149.
     */
    struct FadeAt
    {
        bool active{false};
        int partnerPos{0};
        float mainGain{1.f}, partnerGain{0.f};
    };

    /*
     * Two crossfades, two gain laws, for a reason.
     *
     * Across a wrap the two streams are unrelated material, so what matters is that the
     * power stays put. getFadeGainToAmp is concave, and applying it to both sides sums to
     * more than one in amplitude but close to one in power, which suits that case and is
     * the long-standing behaviour.
     *
     * Across a ping-pong turnaround the two streams are a signal and its own reflection.
     * At the bound they are literally the same sample and near it they are strongly
     * correlated, so they add coherently and it is amplitude that has to be preserved.
     * The same concave law would put a factor of 4/3 - about 2.5dB - on every single
     * turnaround. Linear is also what the Kontakt and HALion references use.
     */
    auto wrapGains = [](float g) -> std::pair<float, float> {
        return {getFadeGainToAmp(1.f - g), getFadeGainToAmp(g)};
    };
    auto mirrorGains = [](float g) -> std::pair<float, float> { return {1.f - g, g}; };

    auto fadeStateAt = [&](int p) -> FadeAt {
        if (loopFade <= 0 || !loopIsContinuing())
            return {};

        if constexpr (loopForward)
        {
            /*
             * The seam is the wrap from endLoop back to startLoop. Over the last
             * loopFade samples of the loop the output morphs into the material
             * immediately preceding startLoop, which is what playback continues with
             * after the wrap.
             */
            if (p <= fadeLo || p > GD->loopUpperBound)
                return {};
            /*
             * Travelling toward the seam the window is the approach and we always
             * fade. Travelling away from it the window is the departure, and there is
             * only something to smooth once a wrap has actually happened - otherwise
             * the first reverse descent past endLoop would jump straight to gain ~1
             * and play the pre-loop material with no seam to hide.
             */
            if (Travel < 0 && !GD->hasLooped)
                return {};
            auto [mg, pg] = wrapGains((float)(p - fadeLo) / (float)loopFade);
            return {true, GD->loopLowerBound - (GD->loopUpperBound - p), mg, pg};
        }
        else
        {
            /*
             * A ping-pong loop has no discontinuity at its bounds - the playhead
             * reverses, so the value is continuous - but it does audibly mirror the
             * waveform. The crossfade blends the playhead against its reflection in the
             * turn, and the two are the same sample there, so the blend is continuous
             * whatever the gain.
             */
            const int half = fadeHalf;
            if (half <= 0)
                return {};

            int bound;
            if (std::abs(p - mirrorHi) <= half)
                bound = mirrorHi;
            else if (std::abs(p - mirrorLo) <= half)
                bound = mirrorLo;
            else
                return {};

            // negative while approaching the bound, positive once past it
            const int past = Travel * (p - bound);
            // nothing to blend against until it has turned somewhere: outside the turn
            // range it is still on its way in, and a voice starting at the sample start
            // walks the whole lower window to get there
            if (!GD->hasLooped && (past > 0 || p < mirrorLo || p > mirrorHi))
                return {};

            /*
             * The two reads swap roles at the bound: the one arriving as the playhead
             * leaves as the reflection, and the reflection arriving becomes the
             * playhead. So the blend turns on the distance from the bound and not on
             * which side of it we are. That gives each of the two streams one monotone
             * ramp across the whole window - which is what the references do - and
             * leaves the mix on the playhead at both edges, so the window can end
             * without a step. Signing it instead lands the mix on the reflection at
             * the far edge, a whole fade away from where playback continues.
             *
             * Scaled by half rather than by loopFade so an odd fade still reaches
             * exactly zero at the edge the window test uses, and does not leave a
             * step of a half sample's worth of gain behind it.
             */
            auto g = std::clamp(0.5f * (1.f - std::abs((float)past) / (float)half), 0.f, 0.5f);
            auto [mg, pg] = mirrorGains(g);
            return {true, 2 * bound - p, mg, pg};
        }
    };

    /*
     * Point the main and crossfade reads at p. The fade pointer is refreshed whenever the
     * fade is running, including on the loop-end-buffer path; it used to live only on the
     * else of that branch, so it froze whenever the loop ended within a FIR window of the
     * end of the wave - which is the common case, and precisely where the partner
     * dominates the output.
     */
    auto refreshReads = [&](int p, const FadeAt &fade) {
        if constexpr (fp)
        {
            // See comment above - the generator wants an FIRoffset centered data set
            readSampleLF32 = SampleDataFL + p - FIRoffset;
            if (stereo)
                readSampleRF32 = SampleDataFR + p - FIRoffset;
        }
        else
        {
            readSampleL = SampleDataL + p - FIRoffset;
            if (stereo)
                readSampleR = SampleDataR + p - FIRoffset;
        }

        if constexpr (loopActive)
        {
            if (fade.active)
            {
                const auto fadeSamplePos = fade.partnerPos;
                if constexpr (fp)
                {
                    readFadeSampleLF32 = SampleDataFL + fadeSamplePos - FIRoffset;
                    if (stereo)
                        readFadeSampleRF32 = SampleDataFR + fadeSamplePos - FIRoffset;
                }
                else
                {
                    readFadeSampleL = SampleDataL + fadeSamplePos - FIRoffset;
                    if (stereo)
                        readFadeSampleR = SampleDataR + fadeSamplePos - FIRoffset;
                }
            }

            // we need both checks because if we are just doing a post-release playdown
            // we don't want to re-pad
            if (p >= WaveSize - resampFIRSize && p <= GD->loopUpperBound)
            {
                for (int k = 0; k < resampFIRSize; ++k)
                {
                    auto q = loopEndIndex(k, p, GD->loopUpperBound, WaveSize, LoopOffset);
                    if constexpr (fp)
                    {
                        loopEndBufferLF32[k] = SampleDataFL[q];
                        if (stereo)
                            loopEndBufferRF32[k] = SampleDataFR[q];
                    }
                    else
                    {
                        loopEndBufferL[k] = SampleDataL[q];
                        if (stereo)
                            loopEndBufferR[k] = SampleDataR[q];
                    }
                }
                if constexpr (fp)
                {
                    readSampleLF32 = loopEndBufferLF32;
                    if (stereo)
                        readSampleRF32 = loopEndBufferRF32;
                }
                else
                {
                    readSampleL = loopEndBufferL;
                    if (stereo)
                        readSampleR = loopEndBufferR;
                }
            }
        }
    };

    // refreshReads points the reads at the playhead, so every path needs it; only the
    // fade it blends against is loop-only
    FadeAt fade{};
    if constexpr (loopActive)
        fade = fadeStateAt(SamplePos);
    refreshReads(SamplePos, fade);

    int NSamples = GD->blockSize;

    int i{0};
    for (i = 0; i < NSamples && !IsFinished; i++)
    {
#define KPStereo(E, T, C, dataL, dataR, fadeL, fadeR)                                              \
    KernelProcessor<E, T, C, loopActive> kp{                                                       \
        SamplePos,        SampleSubPos,       int32_t(m0), i,                                      \
        {dataL, dataR},   {fadeL, fadeR},     fade.active, fade.mainGain,                          \
        fade.partnerGain, {OutputL, OutputR}, IO};                                                 \
    kp.ProcessKernel(GD);

#define KPMono(E, T, C, data, fadeData)                                                            \
    KernelProcessor<E, T, C, loopActive> ks{                                                       \
        SamplePos,   SampleSubPos,  int32_t(m0),      i,         {data}, {fadeData},               \
        fade.active, fade.mainGain, fade.partnerGain, {OutputL}, IO};                              \
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
            case InterpolationTypes::ZOHAA:
            {
                KPStereo(InterpolationTypes::ZOHAA, type_from_cond, 2, readL, readR, readFadeL,
                         readFadeR);
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
            case InterpolationTypes::ZOHAA:
            {
                KPMono(InterpolationTypes::ZOHAA, type_from_cond, 1, readL, readFadeL);
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
                SCLOG_IF(debug, "GENERATOR " << SCD(mxOut) << " " << SCD(mnOut));
                printEvery = 0;
                mxOut = std::numeric_limits<float>::min();
                mnOut = std::numeric_limits<float>::max();
            }
            printEvery++;
        }
#endif

        // 3. Forward sample position
        // wide so a ratio past 128x cannot overflow
        int64_t subPos = SampleSubPos + Ratio * Travel;
        auto incr = (int)(subPos >> 24);
        SamplePos += incr;
        SampleSubPos = (int)(subPos - ((int64_t)incr << 24));

        if constexpr (!loopActive) // these constexprs just remind us not to refactor to break ce
        {
            // No loop, simple case: Play the bounds then done.
            if (SamplePos > GD->playbackUpperBound)
            {
                SamplePos = GD->playbackUpperBound;
                SampleSubPos = 0;
                if (Travel == 1)
                    IsFinished = true;
            }
            if (SamplePos < GD->playbackLowerBound)
            {
                SamplePos = GD->playbackLowerBound;
                SampleSubPos = 0;
                if (Travel == -1)
                    IsFinished = true;
            }
        }
        else if constexpr (!loopWhileGated && loopForward)
        {
            int offset = SamplePos;

            if (Travel > 0)
            {
                // Upper
                if (offset > GD->loopUpperBound)
                {
                    offset -= LoopOffset;
                    GD->loopCount++;
                    GD->hasLooped = true;
                }
            }
            else
            {
                // Lower
                if (offset < GD->loopLowerBound)
                {
                    offset += LoopOffset;
                    GD->loopCount++;
                    GD->hasLooped = true;
                }
            }

            if (offset > WaveSize || offset < 0)
                offset = GD->loopUpperBound;

            SamplePos = offset;
        }
        else if constexpr (!loopWhileGated && !loopForward)
        {
            // bidirectional
            reflect();
            SamplePos = std::clamp(SamplePos, 0, WaveSize);
        }
        else if constexpr (loopForward)
        {
            // gated forward
            if (GD->gated)
            {
                int offset = SamplePos;

                if (Travel > 0)
                {
                    if (offset > GD->loopUpperBound)
                    {
                        offset -= LoopOffset;
                        GD->loopCount++;
                        GD->hasLooped = true;
                    }
                }
                else
                {
                    if (offset < GD->loopLowerBound)
                    {
                        offset += LoopOffset;
                        GD->loopCount++;
                        GD->hasLooped = true;
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
                    if (Travel == 1)
                        IsFinished = true;
                }
                if (SamplePos < GD->playbackLowerBound)
                {
                    SamplePos = GD->playbackLowerBound;
                    SampleSubPos = 0;
                    if (Travel == -1)
                        IsFinished = true;
                }
            }
        }
        else if constexpr (!loopForward && loopWhileGated)
        {
            // gated bidirecational
            if (GD->gated || (LoopDir != GD->directionAtOutset))
            {
                reflect();
                SamplePos = std::clamp(SamplePos, 0, WaveSize);
            }
            else
            {
                // TODO : Careful with releasing a loop while going backwards
                if (SamplePos > GD->playbackUpperBound)
                {
                    SamplePos = GD->playbackUpperBound;
                    SampleSubPos = 0;
                    if (Travel == 1)
                        IsFinished = true;
                }
                if (SamplePos < GD->playbackLowerBound)
                {
                    SamplePos = GD->playbackLowerBound;
                    SampleSubPos = 0;
                    if (Travel == -1)
                        IsFinished = true;
                }
            }
        }

        if constexpr (loopActive)
            fade = fadeStateAt(SamplePos);
        refreshReads(SamplePos, fade);
    }

    // Clean up any items left
    for (; i < NSamples; ++i)
    {
        OutputL[i] = 0.f;
        if (stereo)
            OutputR[i] = 0.f;
    }

    GD->travelDirection = Travel;
    GD->loopDirection = LoopDir;
    GD->samplePos = SamplePos;
    GD->sampleSubPos = SampleSubPos;
    GD->isFinished = IsFinished;

    if constexpr (loopActive)
    {
        /*
         * Inside the loop region, not merely past its lower bound. A reverse voice
         * starts at playbackUpperBound, which is above the loop, so the old test made it
         * "in loop" on its very first block and bumped loopCount from -1 to 0 before any
         * loop had been entered - which fed the Is Looping and Loop Count modulation
         * sources and the loop-count play mode. A released gated loop running out into
         * the tail is past the upper bound and likewise is no longer looping.
         */
        // the turn points, since a ping-pong fade moves the whole excursion half a fade down
        const bool withinLoopBounds = SamplePos >= mirrorLo && SamplePos <= mirrorHi;

        GD->positionWithinLoop =
            std::clamp((SamplePos - mirrorLo) * GD->loopInvertedBounds, 0.f, 1.f);

        if (!loopWhileGated)
            GD->isInLoop = withinLoopBounds;
        else
            GD->isInLoop =
                withinLoopBounds && (GD->gated || (GD->directionAtOutset != GD->loopDirection));

        if (GD->isInLoop && GD->loopCount < 0)
            GD->loopCount = 0;
    }
}
} // namespace scxt::dsp
