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

#include "sample_analytics.h"
#include <limits>
#include <cmath>

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
            ms += powf(sample, 2) * divisor_recip;
        }
    }

    if (s->getSampleLength() > 0)
    {
        return sqrt(ms);
    }

    // What should the RMS of an empty sample be?
    return 0.0f;
}
} // namespace scxt::dsp::sample_analytics
