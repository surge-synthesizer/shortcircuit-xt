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

#ifndef SCXT_SRC_DSP_GENERATOR_H
#define SCXT_SRC_DSP_GENERATOR_H
#include <cstdint>
#include "configuration.h"

namespace scxt::dsp
{
struct GeneratorState
{
    int16_t direction{0};
    int32_t samplePos{0};
    int32_t sampleSubPos{0};
    int32_t lowerBound{0};     // inclusive
    int32_t upperBound{1};     // inclusive
    float invertedBounds{1.f}; // 1 / (UB-LB)
    int32_t ratio{1 << 24};
    int16_t blockSize{scxt::blockSize};
    bool isFinished{true};
    int sampleStart{0};
    int sampleStop{0};
    bool gated{0};

    float positionWithinLoop{0};
    bool isInLoop{false};
};

struct GeneratorIO
{
    float *__restrict outputL{nullptr};
    float *__restrict outputR{nullptr};
    void *__restrict sampleDataL{nullptr};
    void *__restrict sampleDataR{nullptr};
    int waveSize{0};
    void *voicePtr{nullptr}; // TODO: Is this used? Why?
};

enum GeneratorSampleModes
{
    GSM_Normal = 0, // can stop voice
    GSM_Loop = 1,
    GSM_Bidirectional = 2,
    GSM_Shot = 3, // can stop voice
    GSM_LoopUntilRelease = 4
};

typedef void (*GeneratorFPtr)(GeneratorState *__restrict, GeneratorIO *__restrict);
GeneratorFPtr GetFPtrGeneratorSample(bool isStereo, bool isFloat, int LoopMode);

} // namespace scxt::dsp
#endif // SCXT_SRC_DSP_GENERATOR_H
