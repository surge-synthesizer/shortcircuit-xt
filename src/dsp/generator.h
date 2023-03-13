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
    GSM_Normal = 0, // wont stop voice
    GSM_Loop = 1,
    GSM_Bidirectional = 2,
    GSM_Shot = 3, // will stop voice
    GSM_LoopUntilRelease = 4
};

typedef void (*GeneratorFPtr)(GeneratorState *__restrict, GeneratorIO *__restrict);
// TODO Loop Mode should be an enum
GeneratorFPtr GetFPtrGeneratorSample(bool isStereo, bool isFloat, int LoopMode);

} // namespace scxt::dsp
#endif // SCXT_SRC_DSP_GENERATOR_H
