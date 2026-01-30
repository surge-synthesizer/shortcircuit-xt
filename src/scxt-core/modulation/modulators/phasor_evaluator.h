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
    std::array<float, scxt::phasorsPerGroupOrZone> rphase;

    void attack(sst::basic_blocks::modulators::Transport &transport,
                modulation::MiscSourceStorage &rs, sst::basic_blocks::dsp::RNG &rng)
    {
        lastAttackTime = transport.timeInBeats;
        for (int i = 0; i < scxt::phasorsPerGroupOrZone; i++)
        {
            rphase[i] = rng.unif01();
        }
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
                rat *= 3.0 / 2.0; // an eighth note triplet is 1/3 of a beat not 1/2
                break;
            case PhasorStorage::DOTTED:
                rat *= 2.0 / 3.0;
                break;
            case PhasorStorage::NOTE:
                break;
            case PhasorStorage::X_BPM:
                rat *= 4.0; // since its always a quarter note
                break;
            }
            auto tm = transport.timeInBeats;
            if (ps.syncMode == PhasorStorage::VOICEPOS)
            {
                tm -= lastAttackTime;
            }
            auto ph = rs.phasors[i].phase;
            if (ph > 0.99)
                ph = rphase[i];

            auto pr{0.f};
            switch (ps.direction)
            {
            case PhasorStorage::ASCENDING:
                pr = (tm + ph) * rat;
                outputs[i] = pr - std::floor(pr);
                break;
            case PhasorStorage::DESCENDING:
                pr = (tm - ph) * rat;
                outputs[i] = pr - std::ceil(pr);
                break;
            }
        }
    }
};
} // namespace scxt::modulation::modulators
#endif // PHASOR_EVALUATOR_H
