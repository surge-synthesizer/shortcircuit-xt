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

#ifndef SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_PHASOR_EVALUATOR_H
#define SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_PHASOR_EVALUATOR_H

#include <array>
#include "configuration.h"
#include "sst/basic-blocks/dsp/RNG.h"
#include "modulation/modulator_storage.h"

namespace scxt::modulation::modulators
{
struct PhasorEvaluator
{
    std::array<float, scxt::phasorsPerGroupOrZone> outputs;

    double lastAttackTime{0};

    void attack(sst::basic_blocks::modulators::Transport &transport,
                modulation::MiscSourceStorage &rs)
    {
        lastAttackTime = transport.timeInBeats;
    }

    void step(sst::basic_blocks::modulators::Transport &transport,
              modulation::MiscSourceStorage &rs)
    {
        for (int i = 0; i < scxt::phasorsPerGroupOrZone; i++)
        {
            auto ps = rs.phasors[i];
            // this is upside down to spare a divide later
            auto rat = 0.25 * ps.denominator / ps.numerator;
            switch (ps.division)
            {
            case PhasorStorage::TRIPLET:
                rat *= 3.0 / 2.0; // 1/8 node triplet is 1/3 of a beta not 1/2 a beat
                break;
            case PhasorStorage::DOTTED:
                rat *= 2.0 / 3.0;
                break;
            case PhasorStorage::NOTE:
                break;
            }
            auto tm = transport.timeInBeats;
            if (ps.syncMode == PhasorStorage::VOICEPOS)
            {
                tm -= lastAttackTime;
            }

            auto pr = tm * rat;
            outputs[i] = pr - std::floor(pr);
        }
    }
};
} // namespace scxt::modulation::modulators
#endif // PHASOR_EVALUATOR_H
