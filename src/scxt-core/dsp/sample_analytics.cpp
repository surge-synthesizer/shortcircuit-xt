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

#include "sample_analytics.h"
#include <algorithm>
#include <limits>
#include <cmath>
#include <cstring>

namespace scxt::dsp::sample_analytics
{
float computePeak(const std::shared_ptr<sample::Sample> &s)
{
    float peak = 0.0f;
    for (size_t i = 0; i < s->getSampleLength(); i++)
    {
        for (int chan = 0; chan < s->channels; chan++)
        {
            float sample = 0.0;
            switch (s->bitDepth)
            {
            case sample::Sample::BD_I16:
                sample = static_cast<float>(s->GetSamplePtrI16(chan)[i]) /
                         std::numeric_limits<int16_t>::max();
                break;
            case sample::Sample::BD_F32:
                sample = s->GetSamplePtrF32(chan)[i];
                break;
            }
            peak = std::max(peak, std::abs(sample));
        }
    }
    return peak;
}

float computeMaxRMSInBlock(const std::shared_ptr<sample::Sample> &s)
{
    static constexpr size_t rmsb{64};
    size_t idx{0};
    float buf[rmsb];
    memset(buf, 0, sizeof(buf));

    float maxv{0};
    for (size_t i = 0; i < s->getSampleLength(); i++)
    {
        auto pv = buf[idx];
        buf[idx] = 0.f;
        for (int chan = 0; chan < s->channels; chan++)
        {
            float sample = 0.0;
            switch (s->bitDepth)
            {
            case sample::Sample::BD_I16:
                sample = static_cast<float>(s->GetSamplePtrI16(chan)[i]) /
                         std::numeric_limits<int16_t>::max();
                break;
            case sample::Sample::BD_F32:
                sample = s->GetSamplePtrF32(chan)[i];
                break;
            }
            buf[idx] += sample * sample;
        }

        maxv = std::max(maxv, maxv - pv + buf[idx]);
        idx = (idx + 1) & (rmsb - 1);
    }

    return std::sqrt(maxv) / rmsb / s->channels;
}

float computeRMS(const std::shared_ptr<sample::Sample> &s)
{
    float ms = 0.0f;
    const float divisor_recip =
        1.0f / (static_cast<float>(s->channels) * static_cast<float>(s->getSampleLength()));
    for (size_t i = 0; i < s->getSampleLength(); i++)
    {
        for (int chan = 0; chan < s->channels; chan++)
        {
            float sample = 0.0;
            switch (s->bitDepth)
            {
            case sample::Sample::BD_I16:
                sample = static_cast<float>(s->GetSamplePtrI16(chan)[i]) /
                         std::numeric_limits<int16_t>::max();
                break;
            case sample::Sample::BD_F32:
                sample = s->GetSamplePtrF32(chan)[i];
                break;
            }
            ms += std::pow(sample, 2.0f) * divisor_recip;
        }
    }

    if (s->getSampleLength() > 0)
    {
        return std::sqrt(ms);
    }

    // What should the RMS of an empty sample be?
    return 0.0f;
}

int64_t nearestZeroCrossing(const std::shared_ptr<sample::Sample> &s, int64_t pos, int64_t lo,
                            int64_t hi)
{
    if (!s || s->channels == 0)
        return -1;

    auto len = (int64_t)s->getSampleLength();
    lo = std::max(lo, (int64_t)0);
    hi = std::min(hi, len - 1);
    if (lo > hi)
        return -1;
    pos = std::clamp(pos, lo, hi);

    // only sign and relative size matter, so the raw sum needs no scaling
    auto frame = [&s](int64_t i) {
        float res{0};
        for (int chan = 0; chan < s->channels; chan++)
        {
            switch (s->bitDepth)
            {
            case sample::Sample::BD_I16:
                res += s->GetSamplePtrI16(chan)[i];
                break;
            case sample::Sample::BD_F32:
                res += s->GetSamplePtrF32(chan)[i];
                break;
            }
        }
        return res;
    };

    auto crosses = [&frame, len](int64_t i) {
        auto v = frame(i);
        if (v == 0)
            return true;
        for (auto n : {i - 1, i + 1})
        {
            if (n < 0 || n >= len)
                continue;
            auto nv = frame(n);
            if ((v < 0) != (nv < 0) && std::abs(v) <= std::abs(nv))
                return true;
        }
        return false;
    };

    for (int64_t d = 0; pos - d >= lo || pos + d <= hi; ++d)
    {
        if (pos - d >= lo && crosses(pos - d))
            return pos - d;
        if (d > 0 && pos + d <= hi && crosses(pos + d))
            return pos + d;
    }
    return -1;
}
} // namespace scxt::dsp::sample_analytics
