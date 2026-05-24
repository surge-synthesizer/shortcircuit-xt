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

#include "perf_generator.h"

#include <algorithm>
#include <cmath>

namespace scxt::perf
{

namespace
{
int64_t blockAt(double tSec, double sampleRate, int blockSize)
{
    return (int64_t)std::floor(tSec * sampleRate / blockSize);
}

void emitNote(std::vector<std::pair<int64_t, MidiEvent>> &out, double onT, double offT, int channel,
              int key, float vel, double sampleRate, int blockSize)
{
    out.emplace_back(blockAt(onT, sampleRate, blockSize),
                     MidiEvent{MidiEvent::NoteOn, (int16_t)channel, (int16_t)key, vel});
    out.emplace_back(blockAt(offT, sampleRate, blockSize),
                     MidiEvent{MidiEvent::NoteOff, (int16_t)channel, (int16_t)key, vel});
}
} // namespace

BakedSequence bake(const SequenceSpec &spec, double sampleRate, int blockSize, double tailSilenceS)
{
    BakedSequence b;
    b.sampleRate = sampleRate;

    const auto &p = spec.params;
    const float vel = p.velocity / 127.f;
    double lastEdge = 0;

    switch (spec.preset)
    {
    case SequenceSpec::SingleNote:
    {
        // single repeated key (default 60) — on noteLenS, off gapS, repeated N times
        int key = p.lo;
        for (int i = 0; i < p.repeats; ++i)
        {
            double on = i * (p.noteLenS + p.gapS);
            double off = on + p.noteLenS;
            emitNote(b.events, on, off, p.channel, key, vel, sampleRate, blockSize);
            lastEdge = off;
        }
        break;
    }
    case SequenceSpec::CMajorChord:
    {
        for (int i = 0; i < p.repeats; ++i)
        {
            double on = i * (p.noteLenS + p.gapS);
            double off = on + p.noteLenS;
            for (int k : p.chordKeys)
                emitNote(b.events, on, off, p.channel, k, vel, sampleRate, blockSize);
            lastEdge = off;
        }
        break;
    }
    case SequenceSpec::ChromaticBlast:
    {
        int i = 0;
        for (int key = p.lo; key <= p.hi; ++key, ++i)
        {
            double on = i * (p.noteLenS + p.gapS);
            double off = on + p.noteLenS;
            emitNote(b.events, on, off, p.channel, key, vel, sampleRate, blockSize);
            lastEdge = off;
        }
        break;
    }
    case SequenceSpec::Custom:
    {
        for (const auto &e : spec.custom)
        {
            b.events.emplace_back(blockAt(e.t, sampleRate, blockSize),
                                  MidiEvent{e.noteOn ? MidiEvent::NoteOn : MidiEvent::NoteOff,
                                            (int16_t)e.channel, (int16_t)e.key,
                                            e.velocity / 127.f});
            lastEdge = std::max(lastEdge, e.t);
        }
        break;
    }
    }

    std::stable_sort(b.events.begin(), b.events.end(),
                     [](const auto &a, const auto &b) { return a.first < b.first; });
    b.totalBlocks = blockAt(lastEdge + tailSilenceS, sampleRate, blockSize);
    return b;
}

} // namespace scxt::perf
