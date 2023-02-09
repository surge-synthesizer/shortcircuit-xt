/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/

#include "generator.h"
#include "infrastructure/sse_include.h"

#include "resampling.h"
#include "sinc_table.h"
#include <iostream>
#include <algorithm>

#include "sst/basic-blocks/mechanics/simd-ops.h"

namespace scxt::dsp
{
const float I16InvScale = (1.f / (16384.f * 32768.f));
const __m128 I16InvScale_m128 = _mm_set1_ps(I16InvScale);

template <bool stereo, bool floatingPoint, int loopMode>
void GeneratorSample(GeneratorState *__restrict GD, GeneratorIO *__restrict IO);

GeneratorFPtr GetFPtrGeneratorSample(bool Stereo, bool Float, int LoopMode)
{
    if (Stereo)
    {
        if (Float)
        {
            switch (LoopMode)
            {
            case 0:
                return GeneratorSample<1, 1, 0>;
            case 1:
                return GeneratorSample<1, 1, 1>;
            case 2:
                return GeneratorSample<1, 1, 2>;
            case 3:
                return GeneratorSample<1, 1, 3>;
            case 4:
                return GeneratorSample<1, 1, 4>;
            }
        }
        else
        {
            switch (LoopMode)
            {
            case 0:
                return GeneratorSample<1, 0, 0>;
            case 1:
                return GeneratorSample<1, 0, 1>;
            case 2:
                return GeneratorSample<1, 0, 2>;
            case 3:
                return GeneratorSample<1, 0, 3>;
            case 4:
                return GeneratorSample<1, 0, 4>;
            }
        }
    }
    else
    {
        if (Float)
        {
            switch (LoopMode)
            {
            case 0:
                return GeneratorSample<0, 1, 0>;
            case 1:
                return GeneratorSample<0, 1, 1>;
            case 2:
                return GeneratorSample<0, 1, 2>;
            case 3:
                return GeneratorSample<0, 1, 3>;
            case 4:
                return GeneratorSample<0, 1, 4>;
            }
        }
        else
        {
            switch (LoopMode)
            {
            case 0:
                return GeneratorSample<0, 0, 0>;
            case 1:
                return GeneratorSample<0, 0, 1>;
            case 2:
                return GeneratorSample<0, 0, 2>;
            case 3:
                return GeneratorSample<0, 0, 3>;
            case 4:
                return GeneratorSample<0, 0, 4>;
            }
        }
    }
    return 0;
}

template <bool stereo, bool fp, int playmode>
void GeneratorSample(GeneratorState *__restrict GD, GeneratorIO *__restrict IO)
{
    int SamplePos = GD->samplePos;
    int SampleSubPos = GD->sampleSubPos;
    int LowerBound = GD->lowerBound;
    int UpperBound = GD->upperBound;
    int IsFinished = GD->isFinished;
    int WaveSize = IO->waveSize;
    int LoopOffset = std::max(1, UpperBound - LowerBound);
    int Ratio = GD->ratio;
    int RatioSign = Ratio < 0 ? -1 : 1;
    Ratio = std::abs(Ratio);
    int Direction = GD->direction * RatioSign;
    short *__restrict SampleDataL;
    short *__restrict SampleDataR;
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
    int NSamples = GD->blockSize;

    for (int i = 0; i < NSamples; i++)
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
            sL4 = _mm_mul_ps(tmp[0], _mm_loadu_ps(&SampleDataFL[SamplePos]));
            sL4 = _mm_add_ps(sL4, _mm_mul_ps(tmp[1], _mm_loadu_ps(&SampleDataFL[SamplePos + 4])));
            sL4 = _mm_add_ps(sL4, _mm_mul_ps(tmp[2], _mm_loadu_ps(&SampleDataFL[SamplePos + 8])));
            sL4 = _mm_add_ps(sL4, _mm_mul_ps(tmp[3], _mm_loadu_ps(&SampleDataFL[SamplePos + 12])));
            sL4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sL4);
            _mm_store_ss(&OutputL[i], sL4);
            if (stereo)
            {
                sR4 = _mm_mul_ps(tmp[0], _mm_loadu_ps(&SampleDataFR[SamplePos]));
                sR4 =
                    _mm_add_ps(sR4, _mm_mul_ps(tmp[1], _mm_loadu_ps(&SampleDataFR[SamplePos + 4])));
                sR4 =
                    _mm_add_ps(sR4, _mm_mul_ps(tmp[2], _mm_loadu_ps(&SampleDataFR[SamplePos + 8])));
                sR4 = _mm_add_ps(sR4,
                                 _mm_mul_ps(tmp[3], _mm_loadu_ps(&SampleDataFR[SamplePos + 12])));
                sR4 = sst::basic_blocks::mechanics::sum_ps_to_ss(sR4);
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
            sL8A = _mm_madd_epi16(tmp, _mm_loadu_si128((__m128i *)&SampleDataL[SamplePos]));
            if (stereo)
                sR8A = _mm_madd_epi16(tmp, _mm_loadu_si128((__m128i *)&SampleDataR[SamplePos]));
            tmp2 = _mm_add_epi16(
                _mm_mulhi_epi16(*((__m128i *)&sincTable.SincOffsetI16[m0 + 8]), lipol0),
                *((__m128i *)&sincTable.SincTableI16[m0 + 8]));
            sL8B = _mm_madd_epi16(tmp2, _mm_loadu_si128((__m128i *)&SampleDataL[SamplePos + 8]));
            if (stereo)
                sR8B =
                    _mm_madd_epi16(tmp2, _mm_loadu_si128((__m128i *)&SampleDataR[SamplePos + 8]));
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

        // if (samplePos > upperBound) samplePos = upperBound;
        switch (playmode)
        {
        case GSM_Normal:
        {
            if (SamplePos > UpperBound)
            {
                SamplePos = UpperBound;
                SampleSubPos = 0;
                IsFinished = true;
            }
            if (SamplePos < LowerBound)
            {
                SamplePos = LowerBound;
                SampleSubPos = 0;
            }
        }
        break;
        case GSM_Loop:
        {
            int offset = SamplePos;

            if (Direction)
            {
                // Upper
                if (offset > UpperBound)
                    offset -= LoopOffset;
            }
            else
            {
                // Lower
                if (offset < LowerBound)
                    offset += LoopOffset;
            }

            if (offset > WaveSize || offset < 0)
                offset = UpperBound;

            SamplePos = offset;
        }
        break;
        case GSM_LoopUntilRelease:
        {
            if (GD->gated)
            {
                auto offset = SamplePos;
                if (Direction)
                {
                    // Upper
                    if (offset > UpperBound)
                        offset -= LoopOffset;
                }
                else
                {
                    // Lower
                    if (offset < LowerBound)
                        offset += LoopOffset;
                }

                if (offset > WaveSize || offset < 0)
                    offset = UpperBound;

                SamplePos = offset;
            }
            else
            {
                // In this mode the Lower/Upper are the loop markers so use the zone sample
                // start/stop
                if (SamplePos > GD->sampleStop)
                {
                    SamplePos = GD->sampleStop;
                    SampleSubPos = 0;
                    IsFinished = true;
                }
                if (SamplePos < GD->sampleStart)
                {
                    SamplePos = GD->sampleStart;
                    SampleSubPos = 0;
                }
            }
        }
        break;
        case GSM_Bidirectional:
        {
            if (SamplePos >= UpperBound)
            {
                Direction = -1;
            }
            // clang-format off
            else if (SamplePos <= LowerBound) { Direction = 1; }
            // clang-format on
            SamplePos = std::clamp(SamplePos, 0, WaveSize);
        }
        break;
        case GSM_Shot:
        {
            if (!((SamplePos >= LowerBound) && (SamplePos <= UpperBound)))
            {
                /*sampler_voice *v = (sampler_voice*)IO->voicePtr;
                if (v) v->uberrelease();*/
                IsFinished = true;
                SamplePos = std::clamp(SamplePos, LowerBound, UpperBound);
            }
        }
        }
    }

    GD->direction = Direction * RatioSign;
    GD->samplePos = SamplePos;
    GD->sampleSubPos = SampleSubPos;
    GD->isFinished = IsFinished;

    switch (playmode)
    {
    case GSM_Loop:
    case GSM_Bidirectional:
        GD->isInLoop = (SamplePos >= GD->lowerBound);
        GD->positionWithinLoop =
            std::clamp((SamplePos - GD->lowerBound) * GD->invertedBounds, 0.f, 1.f);
        break;

    case GSM_LoopUntilRelease:
        if (GD->gated)
        {
            GD->isInLoop = (SamplePos >= GD->lowerBound);
            GD->positionWithinLoop =
                std::clamp((SamplePos - GD->lowerBound) * GD->invertedBounds, 0.f, 1.f);
        }
        else
        {
            GD->isInLoop = false;
            GD->positionWithinLoop =
                std::clamp((SamplePos - GD->lowerBound) * GD->invertedBounds, 0.f, 1.f);
        }
    default:
        break;
    }
}
} // namespace scxt::dsp