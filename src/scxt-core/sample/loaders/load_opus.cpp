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

#include "sample/sample.h"

#if SCXT_USE_OPUS
#include <vector>
#include <algorithm>
#include <opusfile.h>

namespace scxt::sample
{
namespace
{
// Opus always decodes to 48kHz float
static constexpr uint32_t opusSampleRate{48000};

// Shared decode body for the path and memory open variants. Consumes (and frees)
// the OggOpusFile.
bool decodeOpus(Sample *sample, OggOpusFile *of)
{
    auto channels = op_channel_count(of, -1);
    if (channels < 1 || channels > 2)
    {
        sample->addError("Unable to load " + std::to_string(channels) + " channel Opus");
        op_free(of);
        return false;
    }

    auto total = op_pcm_total(of, -1); // samples per channel
    if (total < 0)
    {
        sample->addError("Opus stream is not seekable / has no known length");
        op_free(of);
        return false;
    }

    sample->channels = channels;
    sample->sample_rate = opusSampleRate;
    sample->bitDepth = Sample::BD_F32;
    sample->sampleLengthPerChannel = total;

    sample->allocateF32(0, total);
    if (channels == 2)
        sample->allocateF32(1, total);

    auto *d0 = sample->GetSamplePtrF32(0);
    auto *d1 = channels == 2 ? sample->GetSamplePtrF32(1) : nullptr;

    // ~120ms of interleaved data at 48kHz, the size opusfile recommends
    static constexpr int framesPerRead{5760};
    std::vector<float> buffer(framesPerRead * channels);

    int64_t pos{0};
    while (true)
    {
        int li{0};
        auto got = op_read_float(of, buffer.data(), (int)buffer.size(), &li);
        if (got == OP_HOLE)
            continue; // recoverable - some samples skipped, keep going
        if (got < 0)
        {
            sample->addError("Opus decode failed with code " + std::to_string(got));
            op_free(of);
            return false;
        }
        if (got == 0)
            break; // end of stream

        auto n = std::min((int64_t)got, total - pos);
        if (channels == 1)
        {
            for (int i = 0; i < n; ++i)
                d0[pos + i] = buffer[i];
        }
        else
        {
            for (int i = 0; i < n; ++i)
            {
                d0[pos + i] = buffer[i * 2];
                d1[pos + i] = buffer[i * 2 + 1];
            }
        }
        pos += got;
        if (pos >= total)
            break;
    }

    op_free(of);
    return true;
}
} // namespace

bool Sample::parseOpus(const fs::path &p)
{
    int err{0};
    auto *of = op_open_file(p.u8string().c_str(), &err);
    if (!of)
    {
        addError("Unable to open Opus file (code " + std::to_string(err) + ")");
        return false;
    }

    if (!decodeOpus(this, of))
        return false;

    sample_loaded = true;
    mFileName = p;
    type = Sample::OPUS_FILE;
    instrument = 0;
    region = 0;
    return true;
}

bool Sample::parseOpus(const uint8_t *data, size_t size)
{
    int err{0};
    auto *of = op_open_memory(data, size, &err);
    if (!of)
    {
        addError("Unable to open Opus data (code " + std::to_string(err) + ")");
        return false;
    }

    return decodeOpus(this, of);
}
} // namespace scxt::sample
#else
namespace scxt::sample
{
bool Sample::parseOpus(const fs::path &p)
{
    addError("This version of ShortCircuit has no Opus support");
    return false;
}
bool Sample::parseOpus(const uint8_t *data, size_t size)
{
    addError("This version of ShortCircuit has no Opus support");
    return false;
}
} // namespace scxt::sample
#endif
