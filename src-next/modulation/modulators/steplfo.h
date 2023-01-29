/*
** Shortcircuit XT is Free and Open Source Software
**
** Shortcircuit is made available under the Gnu General Public License, v3.0
** https://www.gnu.org/licenses/gpl-3.0.en.html; The authors of the code
** reserve the right to re-license their contributions under the MIT license in the
** future at the discretion of the project maintainers.
**
** Copyright 2004-2022 by various individuals as described by the git transaction log
**
** All source at: https://github.com/surge-synthesizer/shortcircuit-xt.git
**
** Shortcircuit was a commercial product from 2004-2018, with copyright and ownership
** in that period held by Claes Johanson at Vember Audio. Claes made Shortcircuit
** open source in December 2020.
*/
#pragma once
#include "datamodel/timedata.h"
#include "utils.h"

namespace scxt::modulation::modulators
{

struct StepLFOStorage
{
    float data[32];
    int repeat;
    float rate;
    float smooth;
    float shuffle;
    int temposync;
    int triggermode; // 0 = voice, 1 = freerun/songsync, 2 = random
    int cyclemode;
    int onlyonce;
    // add midi sync capabilities
};

enum LFOPresets
{
    lp_custom = 0,
    lp_clear,
    lp_sine,
    lp_tri,
    lp_square,
    lp_ramp_up,
    lp_ramp_down,
    lp_noise,
    lp_noise_mean3,
    lp_noise_mean5,
    lp_tremolo_tri,
    lp_tremolo_sin,
    n_lfopresets,
};

void load_lfo_preset(LFOPresets preset, StepLFOStorage *settings);
float lfo_ipol(float *step_history, float phase, float smooth, int odd);

struct StepLFO : NonCopyable<StepLFO>, SampleRateSupport
{
  public:
    StepLFO();
    ~StepLFO();
    void assign(StepLFOStorage *settings, const float *rate, datamodel::TimeData *td);
    void sync();
    void process(int samples);
    float output;

  protected:
    long state;
    long state_tminus1;
    float phase, phaseInc;
    void UpdatePhaseIncrement();
    float wf_history[4];
    float ratemult;
    int shuffle_id;
    const float *rate;
    datamodel::TimeData *td;
    StepLFOStorage *settings;
};
} // namespace scxt::modulation::modulators
