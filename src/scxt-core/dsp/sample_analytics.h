/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2025, Various authors, as described in the github
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
#ifndef SCXT_SRC_SCXT_CORE_DSP_SAMPLE_ANALYTICS_H
#define SCXT_SRC_SCXT_CORE_DSP_SAMPLE_ANALYTICS_H

#include <memory>
#include <sample/sample.h>

namespace scxt::dsp::sample_analytics
{
/**
 *
 * @param s the sample to analyze
 * @return Peak of the entire sample
 */
float computePeak(const std::shared_ptr<sample::Sample> &s);

/**
 *
 * @param s the sample to analyze
 * @return The RMS of the entire sample. Be careful though since
 * silence and a blip will have a very low RMS even though it
 * has a very high sort of 'small block peak' RMS
 */
float computeRMS(const std::shared_ptr<sample::Sample> &s);

/**
 *
 * @param s
 * @return The largest RMS in a 64 sample block
 */
float computeMaxRMSInBlock(const std::shared_ptr<sample::Sample> &s);
} // namespace scxt::dsp::sample_analytics

#endif // SCXT_SRC_DSP_SAMPLE_ANALYTICS_H
