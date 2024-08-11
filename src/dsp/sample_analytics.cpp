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

namespace scxt::dsp::sample_analytics {
    float computePeak(const std::shared_ptr<sample::Sample> &s) {
        float peak = 0.0f;
        for(size_t i = 0; i < s->getSampleLength(); i++) {
            for(int chan = 0; chan < s->channels; chan++) {
                peak = std::max(peak, std::abs(*s->GetSamplePtrF32(chan)));
            }
        }
        return peak;
    }

    float computeRMS(const std::shared_ptr<sample::Sample> &s) {
    /*
         *fn rms(data: &[f32]) -> f32 {
        // Calculate the mean square
        let mut ms = 0.0f32;
        for i in 0..data.len() {
         ms += data[i].powi(2) / data.len() as f32;
        }

        return if !data.is_empty() {
            ms.sqrt()
        } else {
            0.0
        };
        }
    */
        float ms = 0.0f;
        const float divisor = static_cast<float>(s->channels) * static_cast<float>(s->getSampleLength());
        for(size_t i = 0; i < s->getSampleLength(); i++) {
            for(int chan = 0; chan < s->channels; chan++) {
                ms += std::powf(*s->GetSamplePtrF32(chan), 2) / divisor;
            }
        }

        if (s->getSampleLength() > 0) {
            return std::sqrtf(ms);
        }

        // What should the RMS of an empty sample be?
        return 0.0f;
    }
}