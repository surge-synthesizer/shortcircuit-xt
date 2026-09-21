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

#ifndef SCXT_SRC_SCXT_CORE_DSP_GENERATOR_H
#define SCXT_SRC_SCXT_CORE_DSP_GENERATOR_H
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>
#include "configuration.h"
#include "string"
#include "utils.h"

namespace scxt::dsp
{

enum InterpolationTypes
{
    Sinc,
    Linear,
    ZOHAA,
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
    case ZOHAA:
        return "zaa";
    case ZeroOrderHold:
        return "zoh";
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

/*
 * There are two directions here and they are not the same thing.
 *
 * loopDirection is what the loop logic tracks: it is seeded from playReverse at
 * note on, flipped at ping-pong turnarounds, and is what directionAtOutset is
 * compared against.
 *
 * travelDirection is loopDirection folded with the sign of ratio, which goes
 * negative under playback-ratio modulation. It is the direction the playhead
 * actually moves, and it is the only one the position advance and the
 * end-of-playback tests may use. Reaching for loopDirection in that code is wrong
 * whenever ratio is negative, which is how six termination tests ended up stalling
 * voices instead of finishing them.
 */
struct GeneratorState
{
    int16_t loopDirection{0};   // +1 for forward, -1 for back. NOT the travel direction.
    int16_t travelDirection{0}; // loopDirection * sign(ratio); derived, written per block
    int32_t samplePos{0};
    int32_t sampleSubPos{0};

    int32_t playbackLowerBound{0};     // inclusive
    int32_t playbackUpperBound{1};     // inclusive
    float playbackInvertedBounds{1.f}; // 1 / (UB-LB)

    int32_t loopLowerBound{0};     // inclusive
    int32_t loopUpperBound{1};     // inclusive
    float loopInvertedBounds{1.f}; // 1 / (UB-LB)
    int64_t ratio{1 << 24};        // 1 << 24 is playback-at-tempo
    int16_t blockSize{scxt::blockSize};
    bool isFinished{true};
    bool gated{0};
    int16_t loopCount{-1};        // if this is positive then we play this many loops no matter what
    int16_t directionAtOutset{1}; // the loopDirection we started with

    float positionWithinLoop{0};
    bool isInLoop{false};

    // has the playhead crossed a loop boundary at least once - wrapped, or turned
    // around in an alternate loop? Retreating from a seam there is nothing to
    // crossfade against until it has.
    bool hasLooped{false};

    int32_t loopFade{0};

    InterpolationTypes interpolationType{InterpolationTypes::Sinc};
};

/*
 * How much of a loop crossfade is actually usable, given where the partner stream has
 * to read from.
 *
 * Both directions fade across the loopFade samples running up to a loop marker, so both
 * need that much ahead of startLoop and neither reads past endLoop. Loop length binds
 * first and sample start second, the order HALion clamps in.
 *
 * The engine and the editor both go through here, so the XF value on screen is the one
 * you hear. The engine used to clamp silently while the editor showed whatever you
 * dialled in.
 */
inline int64_t clampLoopFade(int64_t loopFade, int64_t startSample, int64_t startLoop,
                             int64_t endLoop)
{
    auto f = std::min(std::max((int64_t)0, loopFade), endLoop - startLoop);
    f = std::min(f, startLoop - startSample);
    return std::max((int64_t)0, f);
}

/*
 * Where a ping-pong crossfade's mirrored read sits.
 *
 * The playhead is at pos + sub/2^24, so its reflection in bound is at
 * 2*bound - pos - sub/2^24 - the fraction runs the other way. Reading the mirror at the
 * playhead's own sub-position leaves it up to a whole sample out at any ratio but 1,
 * which is every ratio once the sample rate differs from the engine's.
 */
inline std::pair<int32_t, int32_t> mirrorRead(int32_t pos, int32_t subPos, int32_t bound)
{
    if (subPos == 0)
        return {2 * bound - pos, 0};
    return {2 * bound - pos - 1, (1 << 24) - subPos};
}

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
