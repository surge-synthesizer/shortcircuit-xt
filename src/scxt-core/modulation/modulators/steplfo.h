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

#ifndef SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_STEPLFO_H
#define SCXT_SRC_SCXT_CORE_MODULATION_MODULATORS_STEPLFO_H

#include "utils.h"
#include <array>
#include <vector>
#include <utility>
#include <algorithm>
#include <cmath>
#include <modulation/modulator_storage.h>
#include "sst/basic-blocks/dsp/RNG.h"
#include "sst/basic-blocks/modulators/Transport.h"

#include "modulation/modulator_storage.h"
#include "tuning/equal.h"

namespace scxt::modulation::modulators
{

struct StepLFO : MoveableOnly<StepLFO>, SampleRateSupport
{
  public:
    StepLFO() : underlyingLfo(tuning::equalTuning) {}
    ~StepLFO() = default;

    void onSampleRateChanged() override { underlyingLfo.setSampleRate(sampleRate, sampleRateInv); }
    void assign(modulation::ModulatorStorage *settings, const float *rate,
                sst::basic_blocks::modulators::Transport *td, sst::basic_blocks::dsp::RNG &rng)
    {
        this->settings = settings;
        this->td = td;
        this->rate = rate;
        underlyingLfo.setSampleRate(sampleRate, sampleRateInv);
        underlyingLfo.assign(&settings->stepLfoStorage, *rate, td, rng, settings->temposync);
        setStartPosition(settings->start_phase);
    }

    /*
     * phase01 is a fraction of the whole sequence. songPosSeconds, when non-zero,
     * additionally advances the position by where the song is (SONGPOS trigger).
     */
    void setStartPosition(float phase01, double songPosSeconds = 0.0)
    {
        if (!settings || !rate)
            return;

        const auto &ss = settings->stepLfoStorage;
        auto repeat = std::max((int)ss.repeat, 1);
        double total = std::clamp(phase01, 0.f, 0.999999f) * repeat;

        if (songPosSeconds != 0.0)
        {
            // Steps advance at 2^rate per second, scaled by the whole-sequence length in
            // cycle mode and by tempo when synced. Mirrors UpdatePhaseIncrement exactly,
            // table for table, so the lock and the free-run don't drift apart.
            double stepsPerSec = (ss.rateIsForSingleStep ? 1.0 : repeat);
            if (settings->temposync && td)
            {
                stepsPerSec *= std::pow(2.0, (double)*rate) * td->tempo / 120.0;
            }
            else
            {
                stepsPerSec *= tuning::equalTuning.note_to_pitch(12 * *rate);
            }
            total += songPosSeconds * stepsPerSec;
        }

        auto totalFloor = (long)std::floor(total);
        auto step = (int)(((totalFloor % repeat) + repeat) % repeat);
        underlyingLfo.setPhaseTo(step, (float)(total - totalFloor));
    }

    void retrigger() { underlyingLfo.retrigger(); }
    void silence() { output = 0.f; }
    // void sync();
    void process(int samples)
    {
        underlyingLfo.process(*rate, settings->triggerMode, settings->temposync,
                              settings->triggerMode == ModulatorStorage::ONESHOT, samples);
        output = underlyingLfo.output;
    }

    float output{0.f};

  private:
    const float *rate{nullptr};
    sst::basic_blocks::modulators::Transport *td{nullptr};
    modulation::ModulatorStorage *settings{nullptr};

    sst::basic_blocks::modulators::StepLFO<scxt::blockSize> underlyingLfo;
};
} // namespace scxt::modulation::modulators

#endif // SCXT_SRC_MODULATION_MODULATORS_STEPLFO_H
