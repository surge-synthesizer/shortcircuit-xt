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
#include <algorithm>
#include <cstring>

using std::max;
using std::min;

extern float SincTableF32[(FIRipol_M + 1) * FIRipol_N];
extern float SincOffsetF32[(FIRipol_M)*FIRipol_N];

/* oscillators			*/
/* pulse				*/

osc_pulse::osc_pulse(float *fp) : filter(fp)
{
    strcpy(filtername, "pulse osc");
    parameter_count = 2;
    strcpy(ctrllabel[0], "tune");
    ctrlmode[0] = cm_mod_pitch;
    strcpy(ctrllabel[1], "width");
    ctrlmode[1] = cm_percent;

    strcpy(ctrlmode_desc[0], str_mpitch);
    strcpy(ctrlmode_desc[1], str_percentdef);

    first_run = true;
    oscstate = 0;
    osc_out = 0;
    polarity = 0;
    bufpos = 0;
    pitch = 0;

    int i;
    for (i = 0; i < ob_length; i++)
        oscbuffer[i] = 0.f;

    if (fp)
    {
    }
}

osc_pulse::~osc_pulse() {}

void osc_pulse::init_params()
{
    if (!param)
        return;
    param[0] = 0.0f;
    param[1] = 0.0f;
}

const int64_t large = 0x10000000000;
const float integrator_hpf = 0.99999999f;

void osc_pulse::convolute()
{
    int ipos = (large + oscstate) >> 16;
    // generate pulse
    float fpol = polarity ? -1.0f : 1.0f;
    int m = ((ipos >> 16) & 0xff) * FIRipol_N;
    float lipol = ((float)((uint32)(ipos & 0xffff)));

    int k;
    for (k = 0; k < FIRipol_N; k++)
    {
        oscbuffer[bufpos + k & (ob_length - 1)] +=
            fpol * (SincTableF32[m + k] + lipol * SincOffsetF32[m + k]);
    }

    // add time until next statechange
    double width = (0.5 - 0.499f * min(1.f, max(0.f, param[1])));
    double t =
        max(0.5, samplerate / (440.0 * pow((double)1.05946309435, (double)pitch + param[0])));
    if (polarity)
    {
        width = 1 - width;
    }
    int64_t rate = (int64_t)(double)(65536.0 * 16777216.0 * t * width);

    oscstate += rate;
    polarity = !polarity;
}

void osc_pulse::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                               float pitch)
{
    process(0, dataoutL, pitch);
    copy_block(dataoutL, dataoutR, block_size_quad);
}
void osc_pulse::process(float *datain, float *dataout, float pitch)
{
    if (first_run)
    {
        first_run = false;
        // initial antipulse
        convolute();
        oscstate -= large;
        int i;
        for (i = 0; i < ob_length; i++)
            oscbuffer[i] *= -0.5f;
        oscstate = 0;
        polarity = 0;
    }

    this->pitch = pitch;
    int k;
    for (k = 0; k < block_size; k++)
    {
        oscstate -= large;
        while (oscstate < 0)
            this->convolute();
        osc_out = osc_out * integrator_hpf + oscbuffer[bufpos];
        dataout[k] = osc_out;
        oscbuffer[bufpos] = 0.f;

        bufpos++;
        bufpos = bufpos & (ob_length - 1);
    }
}

/* pulse sync				*/

osc_pulse_sync::osc_pulse_sync(float *fp) : filter(fp)
{
    strcpy(filtername, "pulse osc (sync)");
    parameter_count = 3;
    strcpy(ctrllabel[0], "tune");
    ctrlmode[0] = cm_mod_pitch;
    strcpy(ctrllabel[1], "width");
    ctrlmode[3] = cm_percent;
    strcpy(ctrllabel[2], "sync");
    ctrlmode[4] = cm_mod_pitch;

    strcpy(ctrlmode_desc[0], str_mpitch);
    strcpy(ctrlmode_desc[1], str_percentdef);
    strcpy(ctrlmode_desc[2], "f,0,0.04,96,0,");

    first_run = true;
    oscstate = 0;
    syncstate = 0;
    osc_out = 0;
    polarity = 0;
    bufpos = 0;
    pitch = 0;

    int i;
    for (i = 0; i < ob_length; i++)
        oscbuffer[i] = 0.f;

    if (fp)
    {
    }
}

osc_pulse_sync::~osc_pulse_sync() {}

void osc_pulse_sync::init_params()
{
    if (!param)
        return;
    param[0] = 0.0f;
    param[1] = 0.0f;
    param[2] = 19.0f;
}

void osc_pulse_sync::convolute()
{

    int ipos = ((large + oscstate) >> 16) & 0xFFFFFFFF;
    bool sync = false;
    if (syncstate < oscstate)
    {
        ipos = ((large + syncstate) >> 16) & 0xFFFFFFFF;
        double t =
            max(0.5, samplerate / (440.0 * pow((double)1.05946309435, (double)pitch + param[0])));
        int64_t syncrate = (int64_t)(double)(65536.0 * 16777216.0 * t);
        oscstate = syncstate;
        syncstate += syncrate;
        sync = true;
    }
    // generate pulse
    float fpol = polarity ? -1.0f : 1.0f;
    int m = ((ipos >> 16) & 0xff) * FIRipol_N;
    float lipol = ((float)((uint32)(ipos & 0xffff)));

    int k;
    if (!sync || !polarity)
    {
        for (k = 0; k < FIRipol_N; k++)
        {
            oscbuffer[bufpos + k & (ob_length - 1)] +=
                fpol * (SincTableF32[m + k] + lipol * SincOffsetF32[m + k]);
        }
    }

    if (sync)
        polarity = false;

    // add time until next statechange
    double width = (0.5 - 0.499f * min(1.f, max(0.f, param[1])));
    double t =
        max(0.5,
            samplerate / (440.0 * pow((double)1.05946309435, (double)pitch + param[0] + param[2])));
    lastpulselength = t;
    if (polarity)
    {
        width = 1 - width;
    }
    int64_t rate = (int64_t)(double)(65536.0 * 16777216.0 * t * width);

    oscstate += rate;
    polarity = !polarity;
}

void osc_pulse_sync::process_stereo(float *datainL, float *datainR, float *dataoutL,
                                    float *dataoutR, float pitch)
{
    process(0, dataoutL, pitch);
    copy_block(dataoutL, dataoutR, block_size_quad);
}
void osc_pulse_sync::process(float *datain, float *dataout, float pitch)
{
    if (first_run)
    {
        first_run = false;

        // initial antipulse
        convolute();
        oscstate -= large;
        int i;
        for (i = 0; i < ob_length; i++)
            oscbuffer[i] *= -0.5f;
        oscstate = 0;
        polarity = 0;
    }
    this->pitch = pitch;
    int k;
    for (k = 0; k < block_size; k++)
    {
        oscstate -= large;
        syncstate -= large;
        while (syncstate < 0)
            this->convolute();
        while (oscstate < 0)
            this->convolute();
        osc_out = osc_out * integrator_hpf + oscbuffer[bufpos];
        dataout[k] = osc_out;
        oscbuffer[bufpos] = 0.f;

        bufpos++;
        bufpos = bufpos & (ob_length - 1);
    }
}

/* sawtooth				*/
osc_saw::osc_saw(float *fp, int *ip) : filter(fp, 0, true, ip)
{
    strcpy(filtername, "sawtooth osc");
    parameter_count = 3;
    strcpy(ctrllabel[0], "tune");
    ctrlmode[0] = cm_mod_pitch;
    strcpy(ctrllabel[1], "detune");
    ctrlmode[1] = cm_mod_pitch;
    strcpy(ctrllabel[2], "ringmod");
    ctrlmode[2] = cm_percent;

    strcpy(ctrlmode_desc[0], str_mpitch);
    strcpy(ctrlmode_desc[1], "f,-96,0.01,96,1,cents");
    strcpy(ctrlmode_desc[2], str_percentdef);

    if (fp && ip) // fp will be valid when called by the voice, but not the editor
    {
        first_run = true;
        osc_out = 0;
        bufpos = 0;
        dc = 0;
        n_unison = iparam[0] + 1;
        n_unison = max(1, min(16, n_unison));
        out_attenuation = 1 / sqrt((float)n_unison);

        if (n_unison == 1)
        {
            detune_bias = 1;
            detune_offset = 0;
        }
        else
        {
            detune_bias = (float)2 / (n_unison);
            detune_offset = -1;
        }

        int i;
        for (i = 0; i < ob_length; i++)
            oscbuffer[i] = 0.f;
    }
}

osc_saw::~osc_saw() {}

void osc_saw::init_params()
{
    if (!param)
        return;
    param[0] = 0.0f;
    param[1] = 0.2f;
    param[2] = 0.0f;
    iparam[0] = 0;
}

void osc_saw::convolute(int voice)
{
    int ipos = (large + oscstate[voice]) >> 16;
    // generate pulse
    int m = ((ipos >> 16) & 0xff) * FIRipol_N;
    float lipol = ((float)((uint32)(ipos & 0xffff)));

    int k;
    float a, s = 0;
    if (dc_uni[voice] == 0.0f)
    {
        for (k = 0; k < FIRipol_N; k++)
        {
            a = SincTableF32[m + k] + lipol * SincOffsetF32[m + k];
            oscbuffer[bufpos + k & (ob_length - 1)] += 0.5f * a;
            s += a;
        }
    }
    else
    {
        for (k = 0; k < FIRipol_N; k++)
        {
            a = SincTableF32[m + k] + lipol * SincOffsetF32[m + k];
            oscbuffer[bufpos + k & (ob_length - 1)] += a;
            s += a;
        }
    }

    // add time until next statechange
    double detune = param[1] * (detune_bias * float(voice) + detune_offset);
    double t = max(2.0, samplerate / (440.0 * pow(1.05946309435, pitch + param[0] + detune)));
    dc_uni[voice] = s / t;
    int64_t rate = (int64_t)(double)(65536.0 * 16777216.0 * t);

    oscstate[voice] += rate;
}

int osc_saw::get_ip_count() { return 1; }

const char *osc_saw::get_ip_label(int ip_id)
{
    if (ip_id == 0)
        return "unison";
    return "";
}

int osc_saw::get_ip_entry_count(int ip_id)
{
    return 5; // 4 poles
}
const char *osc_saw::get_ip_entry_label(int ip_id, int c_id)
{
    if (ip_id == 0)
    {
        switch (c_id)
        {
        case 0:
            return "1";
        case 1:
            return "2";
        case 2:
            return "3";
        case 3:
            return "4";
        case 4:
            return "5";
        }
    }
    return "";
}

void osc_saw::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                             float pitch)
{
    process(datainL, dataoutL, pitch);
    copy_block(dataoutL, dataoutR, block_size_quad);
}
void osc_saw::process(float *datain, float *dataout, float pitch)
{
    if (first_run)
    {
        first_run = false;

        int i;
        for (i = 0; i < n_unison; i++)
        {
            double drand = (double)rand() / RAND_MAX;
            double t = drand * max(2.0, samplerate / (440.0 * pow((double)1.05946309435,
                                                                  (double)pitch + param[0])));
            oscstate[i] = (int64_t)(double)(65536.0 * 16777216.0 * t);
            dc_uni[i] = 0;
        }
    }
    this->pitch = pitch;
    int k, l;
    for (k = 0; k < block_size; k++)
    {
        dc = 0;
        for (l = 0; l < n_unison; l++)
        {
            oscstate[l] -= large;
            while (oscstate[l] < 0)
                this->convolute(l);
            dc += dc_uni[l];
        }

        oscbuffer[(bufpos + 6) & (ob_length - 1)] -= dc;
        osc_out = osc_out * integrator_hpf + oscbuffer[bufpos];
        dataout[k] =
            out_attenuation * (1.f + param[2] * (datain[k] - 1.f)) * osc_out; // ringmod mix
        oscbuffer[bufpos] = 0.f;

        bufpos++;
        bufpos = bufpos & (ob_length - 1);
    }
}

/* sin				*/
osc_sin::osc_sin(float *fp) : filter(fp)
{
    strcpy(filtername, "sinus osc");
    parameter_count = 1;
    strcpy(ctrllabel[0], "tune");
    ctrlmode[0] = cm_mod_pitch;

    strcpy(ctrlmode_desc[0], str_mpitch);

    if (fp) // fp will be valid when called by the voice, but not the editor
    {
    }
}

osc_sin::~osc_sin() {}

void osc_sin::init_params()
{
    if (!param)
        return;
    param[0] = 0.0f;
}

void osc_sin::process(float *datain, float *dataout, float pitch)
{
    osc.set_rate(440.0 * pi2 * pow((double)1.05946309435, (double)pitch + param[0]) *
                 samplerate_inv);

    for (int k = 0; k < block_size; k++)
    {
        dataout[k] = osc.r;
        osc.process();
    }
}

void osc_sin::process_stereo(float *datainL, float *datainR, float *dataoutL, float *dataoutR,
                             float pitch)
{
    osc.set_rate(440.0 * pi2 * pow((double)1.05946309435, (double)pitch + param[0]) *
                 samplerate_inv);

    for (int k = 0; k < block_size; k++)
    {
        dataoutL[k] = osc.r;
        dataoutR[k] = osc.r;
        osc.process();
    }
}
