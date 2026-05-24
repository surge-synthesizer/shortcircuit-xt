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

#ifndef SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_GENERATOR_H
#define SCXT_SRC_CLIENTS_PERF_HARNESS_PERF_GENERATOR_H

#include <cstdint>
#include <utility>
#include <vector>

#include "perf_config.h"

namespace scxt::perf
{

struct MidiEvent
{
    enum Type
    {
        NoteOn,
        NoteOff
    } type;
    int16_t channel;
    int16_t key;
    float velocity;
};

// blockSize is intentionally absent — the engine's block size is a compile-time
// constant (scxt::blockSize). Don't store a runtime copy that pretends to be a
// degree of freedom we can't actually honour.
struct BakedSequence
{
    double sampleRate{0};
    int64_t totalBlocks{0};
    std::vector<std::pair<int64_t, MidiEvent>> events; // sorted by block_idx
};

BakedSequence bake(const SequenceSpec &, double sampleRate, int blockSize, double tailSilenceS);

} // namespace scxt::perf
#endif
