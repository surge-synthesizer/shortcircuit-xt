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

#ifndef SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_RUNNER_H
#define SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_RUNNER_H

#include <cstdint>
#include <vector>

#include "perf_config.h"
#include "perf_generator.h"
#include "perf_fingerprint.h"

namespace scxt::engine
{
struct Engine;
}

namespace scxt::perf
{

struct IterationStats
{
    double wallMs{0};
    double realtimeRatio{0};
    uint32_t peakVoices{0};
    int drainBlocks{0};
    // Block-time samples in nanoseconds (one entry per processAudio call within
    // the timed window). We keep them all for percentile computation.
    std::vector<int64_t> blockNs;
};

struct ScenarioStats
{
    double sampleRate{0};
    int blockSize{0};
    int64_t totalBlocks{0};
    double totalSeconds{0};
};

struct RunResult
{
    ScenarioStats scenario;
    Fingerprint fingerprint; // captured on the cold (first warmup, or first measure) iter
    std::vector<IterationStats> warmupIterations;
    std::vector<IterationStats> measureIterations;
    std::vector<float>
        wavInterleaved; // captured during the same fingerprint iter (if wav requested)
};

RunResult runMeasure(scxt::engine::Engine &, const BakedSequence &, const RunConfig &);
void runProfile(scxt::engine::Engine &, const BakedSequence &, const RunConfig &);

} // namespace scxt::perf
#endif
