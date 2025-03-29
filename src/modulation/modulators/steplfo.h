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

#ifndef SCXT_SRC_MODULATION_MODULATORS_STEPLFO_H
#define SCXT_SRC_MODULATION_MODULATORS_STEPLFO_H

#include "utils.h"
#include <array>
#include <vector>
#include <utility>
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
        underlyingLfo.phase = settings->start_phase;
    }

    void retrigger() { underlyingLfo.retrigger(); }
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
