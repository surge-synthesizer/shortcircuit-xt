//-------------------------------------------------------------------------------------------------------
//
//	Shortcircuit
//
//	Copyright 2004 Claes Johanson
//
//-------------------------------------------------------------------------------------------------------
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
