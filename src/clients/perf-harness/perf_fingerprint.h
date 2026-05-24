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

#ifndef SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_FINGERPRINT_H
#define SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_FINGERPRINT_H

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "perf_fnv.h"

namespace scxt::perf
{

// Two fingerprints per audio scenario:
//  - exact   : FNV-1a over the raw float bits. Bit-identical across runs ⇒ no
//              regression. Bit-different ⇒ audio actually changed (or
//              non-determinism crept in).
//  - tolerant: rolling stats (sample count, sum, sum_sq, peak, zero-crossings)
//              plus per-100ms peak envelope. Comparable across compilers /
//              optimization levels with tolerances.
struct Fingerprint
{
    uint64_t exact{kFnvOffsetBasis};

    uint64_t samples{0};
    double sum{0};
    double sumSq{0};
    double peak{0};
    uint64_t zeroCrossings{0};

    double sampleRate{0};
    int envelopeBucketBlocks{0}; // blocks per 100ms bucket
    int currentBucketBlocks{0};
    float currentBucketPeak{0};
    std::vector<float> envelope;

    float prevSampleL{0};

    void configure(double sr, int blockSize)
    {
        sampleRate = sr;
        envelopeBucketBlocks = std::max(1, (int)std::round(sr * 0.1 / blockSize));
    }

    template <int BlockSize> void absorbBlock(const float (&out)[2][BlockSize])
    {
        for (int ch = 0; ch < 2; ++ch)
            fnv1a(exact, out[ch], sizeof(out[ch]));
        float blockPeak = 0;
        for (int i = 0; i < BlockSize; ++i)
        {
            float l = out[0][i];
            float r = out[1][i];
            sum += l + r;
            sumSq += (double)l * l + (double)r * r;
            float al = std::fabs(l), ar = std::fabs(r);
            if (al > blockPeak)
                blockPeak = al;
            if (ar > blockPeak)
                blockPeak = ar;
            if ((prevSampleL < 0 && l >= 0) || (prevSampleL >= 0 && l < 0))
                zeroCrossings++;
            prevSampleL = l;
        }
        samples += BlockSize * 2;
        if (blockPeak > peak)
            peak = blockPeak;

        if (blockPeak > currentBucketPeak)
            currentBucketPeak = blockPeak;
        currentBucketBlocks++;
        if (currentBucketBlocks >= envelopeBucketBlocks)
        {
            envelope.push_back(currentBucketPeak);
            currentBucketPeak = 0;
            currentBucketBlocks = 0;
        }
    }

    void finalize()
    {
        if (currentBucketBlocks > 0)
        {
            envelope.push_back(currentBucketPeak);
            currentBucketPeak = 0;
            currentBucketBlocks = 0;
        }
    }

    double rms() const { return samples > 0 ? std::sqrt(sumSq / (double)samples) : 0.0; }
};

} // namespace scxt::perf
#endif
