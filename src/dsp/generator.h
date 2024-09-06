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

#ifndef SCXT_SRC_DSP_GENERATOR_H
#define SCXT_SRC_DSP_GENERATOR_H
#include <cstdint>
#include <cstring>
#include "configuration.h"
#include "string"
#include "utils.h"

namespace scxt::dsp
{

enum InterpolationTypes
{
    Sinc,
    Linear,
    ZeroOrderHold
};
DECLARE_ENUM_STRING(InterpolationTypes);

inline std::string toStringInterpolationTypes(const InterpolationTypes &p)
{
    switch (p)
    {
    case Sinc:
        return "sinc";
    case Linear:
        return "lin";
    case ZeroOrderHold:
        return "zho";
    }
    return "sinc";
}

inline InterpolationTypes fromStringInterpolationTypes(const std::string &s)
{
    static auto inverse = makeEnumInverse<InterpolationTypes, toStringInterpolationTypes>(
        InterpolationTypes::Sinc, InterpolationTypes::ZeroOrderHold);
    auto p = inverse.find(s);
    if (p == inverse.end())
        return Sinc;
    return p->second;
}

struct GeneratorState
{
    int16_t direction{0}; // +1 for forward, -1 for back
    int32_t samplePos{0};
    int32_t sampleSubPos{0};

    int32_t playbackLowerBound{0};     // inclusive
    int32_t playbackUpperBound{1};     // inclusive
    float playbackInvertedBounds{1.f}; // 1 / (UB-LB)

    int32_t loopLowerBound{0};     // inclusive
    int32_t loopUpperBound{1};     // inclusive
    float loopInvertedBounds{1.f}; // 1 / (UB-LB)
    int32_t ratio{1 << 24};        // 1 << 24 is playback-at-tempo
    int16_t blockSize{scxt::blockSize};
    bool isFinished{true};
    int32_t sampleStart{0};
    int32_t sampleStop{0};
    bool gated{0};
    int16_t loopCount{-1};        // if this is positive then we play this many loops no matter what
    int16_t directionAtOutset{1}; // is our 'ur-' direction forward or backwards?

    float positionWithinLoop{0};
    bool isInLoop{false};

    int32_t loopFade{0};

    InterpolationTypes interpolationType{InterpolationTypes::Sinc};
};

struct GeneratorIO
{
    float *__restrict outputL{nullptr};
    float *__restrict outputR{nullptr};
    void *__restrict sampleDataL{nullptr};
    void *__restrict sampleDataR{nullptr};
    int waveSize{0};
};

typedef void (*GeneratorFPtr)(GeneratorState *__restrict, GeneratorIO *__restrict);
// TODO Loop Mode should be an enum
GeneratorFPtr GetFPtrGeneratorSample(bool isStereo, bool isFloat, bool loopActive, bool loopForward,
                                     bool loopWhileGated);

} // namespace scxt::dsp
#endif // SCXT_SRC_DSP_GENERATOR_H
