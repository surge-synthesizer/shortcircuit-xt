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

#ifndef SCXT_SRC_DATAMODEL_ADSR_STORAGE_H
#define SCXT_SRC_DATAMODEL_ADSR_STORAGE_H

#include <tuple>
#include "parameter.h"
#include "sst/basic-blocks/modulators/ADSREnvelope.h"

namespace scxt::datamodel
{
struct AdsrStorage
{
    /*
     * Each of the ADR are 0..1 and scaled to time by the envelope. S is a 0..1 pct
     */
    float a{0.0}, h{0.0}, d{0.0}, s{1.0}, r{0.5};
    bool isDigital{true};

    // TODO: What are these going to be when they grow up?
    float aShape{0}, dShape{0}, rShape{0};

    // The names are not stored on these right now. Fix?
    static datamodel::pmd paramAHD, paramR, paramS, paramShape;

    auto asTuple() const { return std::tie(a, d, s, r, isDigital, aShape, dShape, rShape); }
    bool operator==(const AdsrStorage &other) const { return asTuple() == other.asTuple(); }
    bool operator!=(const AdsrStorage &other) const { return !(*this == other); }
};
} // namespace scxt::datamodel

#endif // SHORTCIRCUIT_ADSR_STORAGE_H
