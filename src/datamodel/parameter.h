/*
 * Shortcircuit XT - a Surge Synth Team product
 *
 * A fully featured creative sampler, available as a standalone
 * and plugin for multiple platforms.
 *
 * Copyright 2019 - 2023, Various authors, as described in the github
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

#ifndef SCXT_SRC_DATAMODEL_PARAMETER_H
#define SCXT_SRC_DATAMODEL_PARAMETER_H

#include <string>
#include <optional>

#include "sst/basic-blocks/params/ParamMetadata.h"
#include "sst/basic-blocks/modulators/ADSREnvelope.h"

namespace scxt::datamodel
{
using pmd = sst::basic_blocks::params::ParamMetaData;
inline pmd decibelRange(float lower = -96, float upper = 12)
{
    return pmd()
        .withType(pmd::FLOAT)
        .withRange(lower, upper)
        .withDefault(0)
        .withLinearScaleFormatting("dB");
}

inline pmd semitoneRange(float lower = -96, float upper = 96)
{
    return pmd()
        .withType(pmd::FLOAT)
        .withRange(lower, upper)
        .withDefault(0)
        .withLinearScaleFormatting("semitones");
}

inline pmd secondsLog2Range(float lower, float upper)
{
    return pmd()
        .withType(pmd::FLOAT)
        .withRange(lower, upper)
        .withDefault(std::clamp(0.f, lower, upper))
        .withATwoToTheBFormatting(1, 1, "s");
}

inline pmd pitchTransposition() { return semitoneRange(-96, 96); }

inline pmd pitchInOctavesFromA440()
{
    return pmd()
        .withType(pmd::FLOAT)
        .withRange(-6, 5)
        .withDefault(0)
        .withATwoToTheBFormatting(440, 1, "Hz");
};

/*
 * A 0...1 parameter scaled to display as etMin..etMax in log2 space
 */
inline pmd envelopeThirtyTwo()
{
    return pmd()
        .withType(pmd::FLOAT)
        .withRange(0.f, 1.f)
        .withDefault(0.2f)
        .withATwoToTheBPlusCFormatting(
            1.f,
            sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMax -
                sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMin,
            sst::basic_blocks::modulators::ThirtyTwoSecondRange::etMin, "s");
}

inline pmd lfoModulationRate()
{
    return pmd()
        .withType(pmd::FLOAT)
        .withRange(-3, 8)
        .withDefault(0)
        .withATwoToTheBFormatting(1, 1, "Hz");
}

inline pmd lfoSmoothing()
{
    return pmd().withType(pmd::FLOAT).withRange(0, 2).withDefault(0).withLinearScaleFormatting("");
}

inline pmd unusedParam() { return pmd().withType(pmd::NONE); }

} // namespace scxt::datamodel

#endif // __SCXT_PARAMETER_H
