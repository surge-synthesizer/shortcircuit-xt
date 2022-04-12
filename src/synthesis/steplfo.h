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

struct timedata;
struct steplfostruct;

void load_lfo_preset(int preset, steplfostruct *settings);
float lfo_ipol(float *step_history, float phase, float smooth, int odd);

class steplfo
{
  public:
    steplfo();
    ~steplfo();
    void assign(steplfostruct *settings, float *rate, timedata *td);
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
    float *rate;
    timedata *td;
    steplfostruct *settings;
};
