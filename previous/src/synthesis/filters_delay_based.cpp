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

#include "filter_defs.h"
#include "resampling.h"
#include "util/tools.h"
#include <string.h>

extern float SincTableF32[(FIRipol_M + 1) * FIRipol_N];
extern float SincOffsetF32[(FIRipol_M)*FIRipol_N];

/*	COMB1		*/

COMB1::COMB1(float *fp) : filter(fp)
{
    strcpy(filtername, "comb filter");
    parameter_count = 2;
    strcpy(ctrllabel[0], "frequency");
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], "feedback");
    ctrlmode[1] = cm_percent_bipolar;
    wpos = 0;

    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentbpdef);

    memset(delayloop[0], 0, sizeof(float) * comb_max_delay);
    memset(delayloop[1], 0, sizeof(float) * comb_max_delay);
}

COMB1::~COMB1() {}

void COMB1::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = 0.0f;
    param[1] = 0.0f;
}

void COMB1::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                           float pitch)
{
    /*float p0powf = powf(2,param[0]);
    float dtime = 1.f/(440.f*p0powf);
    dtime = dtime*samplerate - FIRoffset;
    dtime = limit_range(dtime,0,comb_max_delay-FIRipol_N-1);
    delaytime.newValue(dtime);
    feedback.newValue(limit_range(((param[1]>0.f)?1.f:-1.f)*powf(fabs(param[1]),1.f/p0powf),-1.f,1.f));*/

    float dtime = 1 / (440.f * powf(2, param[0]));
    dtime = dtime * samplerate - FIRoffset;
    dtime = limit_range(dtime, 0.f, (float)(comb_max_delay - FIRipol_N - 1));
    delaytime.newValue(dtime);
    feedback.newValue(clamp1bp(param[1]));

    int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        float outputL = 0.f;
        float outputR = 0.f;
        delayloop[0][wpos] = datainL[k];
        delayloop[1][wpos] = datainR[k];

        int i_dt = std::max(0, (int)delaytime.v);
        int sinc = FIRipol_N * limit_range((int)(FIRipol_M * (float(i_dt + 1) - delaytime.v)), 0,
                                           (int)FIRipol_M - 1);

        for (int i = 0; i < FIRipol_N; i++)
        {
            outputL += delayloop[0][(wpos - i_dt - i) & (comb_max_delay - 1)] *
                       SincTableF32[sinc + FIRipol_N - i];
            outputR += delayloop[1][(wpos - i_dt - i) & (comb_max_delay - 1)] *
                       SincTableF32[sinc + FIRipol_N - i];
        }

        delayloop[0][wpos] = tanh_fast(delayloop[0][wpos] + outputL * feedback.v);
        delayloop[1][wpos] = tanh_fast(delayloop[1][wpos] + outputR * feedback.v);

        dataoutL[k] = datainL[k] + outputL;
        dataoutR[k] = datainR[k] + outputR;

        wpos = (wpos + 1) & (comb_max_delay - 1);
        delaytime.process();
        feedback.process();
    }
}

void COMB1::process(float *datain, float *dataout, float pitch)
{
    float dtime = 1 / (440.f * powf(2, param[0]));
    dtime = dtime * samplerate - FIRoffset;
    dtime = limit_range(dtime, 0.f, (float)comb_max_delay - FIRipol_N - 1);
    delaytime.newValue(dtime);
    feedback.newValue(clamp1bp(param[1]));

    int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        float input = datain[k];
        float output = 0.f;
        delayloop[0][wpos] = datain[k];

        int i_dt = std::max(0, (int)delaytime.v);
        int sinc = FIRipol_N * limit_range((int)(FIRipol_M * (float(i_dt + 1) - delaytime.v)), 0,
                                           (int)FIRipol_M - 1);

        for (int i = 0; i < FIRipol_N; i++)
        {
            output += delayloop[0][(wpos - i_dt - i) & (comb_max_delay - 1)] *
                      SincTableF32[sinc + FIRipol_N - i];
        }

        delayloop[0][wpos] = tanh_fast(delayloop[0][wpos] + output * feedback.v);
        dataout[k] = input + output;

        wpos = (wpos + 1) & (comb_max_delay - 1);
        delaytime.process();
        feedback.process();
    }
}

/*	COMB3

        same as COMB1 but the original signal is not mixed and the delay time is set directly
        not intended to be used directly, but as a component in filters/effects

*/

COMB3::COMB3(float *fp) : filter(fp)
{
    strcpy(filtername, "comb filter 3");
    parameter_count = 2;
    strcpy(ctrllabel[0], "delay time");
    ctrlmode[0] = cm_frequency_audible;
    strcpy(ctrllabel[1], "feedback");
    ctrlmode[1] = cm_percent_bipolar;
    wpos = 0;

    strcpy(ctrlmode_desc[0], str_freqdef);
    strcpy(ctrlmode_desc[1], str_percentbpdef);

    memset(delayloop, 0, sizeof(float) * comb_max_delay);
}

COMB3::~COMB3() {}

void COMB3::init_params()
{
    if (!param)
        return;
    // preset values
    param[0] = 0.0f;
    param[1] = 0.0f;
}

void COMB3::process(float *data, float pitch)
{
    float dtime = powf(2, param[0]);
    dtime = dtime * samplerate - FIRoffset;
    dtime = limit_range(dtime, 0.f, (float)(comb_max_delay - FIRipol_N - 1));
    delaytime.newValue(dtime);
    feedback.newValue(clamp1bp(param[1]));

    int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        float output = 0.f;
        delayloop[wpos] = data[k];

        int i_dt = std::max(0, (int)delaytime.v);
        int sinc = FIRipol_N * limit_range((int)(FIRipol_M * (float(i_dt + 1) - delaytime.v)), 0,
                                           (int)FIRipol_M - 1);

        for (int i = 0; i < FIRipol_N; i++)
        {
            output += delayloop[(wpos - i_dt - i) & (comb_max_delay - 1)] *
                      SincTableF32[sinc + FIRipol_N - i];
        }

        delayloop[wpos] = tanh_fast(delayloop[wpos] + output * feedback.v);
        data[k] = output;

        wpos = (wpos + 1) & (comb_max_delay - 1);
        delaytime.process();
        feedback.process();
    }
}
